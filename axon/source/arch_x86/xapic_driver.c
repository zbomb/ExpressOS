/*==============================================================
    Axon Kernel - X86 XAPIC Driver
    2021, Zachary Berry
    axon/source/arch_x86/xapic_driver.c
==============================================================*/

#include "axon/arch_x86/xapic_driver.h"
#include "axon/arch_x86/acpi_info.h"
#include "axon/arch_x86/util.h"
#include "axon/memory/kmap.h"
#include "axon/arch.h"
#include "axon/debug_print.h"
#include "string.h"
#include "stdlib.h"

/*
    Constants
*/
#define LAPIC_REGISTER_ID                       0x20
#define LAPIC_REGISTER_VERSION                  0x30
#define LAPIC_REGISTER_TASK_PRIORITY            0x80
#define LAPIC_REGISTER_ARBITRATION_PRIORITY     0x90
#define LAPIC_REGISTER_PROCESSOR_PRIORITY       0xA0
#define LAPIC_REGISTER_EOI                      0xB0
#define LAPIC_REGISTER_REMOTE_READ              0xC0
#define LAPIC_REGISTER_LOGICAL_DESTINATION      0xD0
#define LAPIC_REGISTER_DESTINATION_FORMAT       0xE0
#define LAPIC_REGISTER_SPURIOUS_VECTOR          0xF0
#define LAPIC_REGISTER_ERROR_STATUS             0x280
#define LAPIC_REGISTER_LVT_CMCI                 0x2F0
#define LAPIC_REGISTER_IPI_PARAMETERS           0x300
#define LAPIC_REGISTER_IPI_DESTINATION          0x310
#define LAPIC_REGISTER_LVT_TIMER                0x320
#define LAPIC_REGISTER_LVT_THERMAL_SENSOR       0x330
#define LAPIC_REGISTER_LVT_PERF_COUNTERS        0x340
#define LAPIC_REGISTER_LVT_INT0                 0x350
#define LAPIC_REGISTER_LVT_INT1                 0x360
#define LAPIC_REGISTER_LVT_ERROR                0x370
#define LAPIC_REGISTER_TIMER_INIT_COUNT         0x380
#define LAPIC_REGISTER_TIMER_CURRENT_COUNT      0x390
#define LAPIC_REGISTER_TIMER_DIVIDE_CONFIG      0x3E0

#define IOAPIC_REGISTER_ID                  0x00
#define IOAPIC_REGISTER_VERSION             0x01
#define IOAPIC_REGISTER_ARBITRATION_ID      0x02
#define IOAPIC_REGISTER_REDIRECTION_TABLE   0x10

#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1
#define ICW1_INIT   0x10
#define ICW1_ICW4   0x01
#define ICW4_8086   0x01

/*
    Forward Decl.
*/

void axk_x86_xapic_init_lapic( struct axk_x86_xapic_driver_t* this, struct axk_x86_lapic_nmi_t* nmi_list, uint32_t nmi_count );
void axk_x86_xapic_init_ioapic( struct axk_x86_xapic_driver_t* this, struct axk_x86_ioapic_nmi_t* nmi_list, uint32_t nmi_count );
void axk_x86_xapic_disable_pic( struct axk_x86_xapic_driver_t* this );

/*
    ASM Functions
*/
void axk_x86_load_idt( void );
void axk_x86_load_idt_aux( void );
void axk_x86_test_idt( void );

/*
    Function Implementations
*/
bool xapic_driver_init( struct axk_interrupt_driver_t* this_generic )
{
    // Cast 'this'
    struct axk_x86_xapic_driver_t* this = (struct axk_x86_xapic_driver_t*)( this_generic );

    // Ensure xAPIC is enabled and were the BSP
    uint64_t apic_msr       = axk_x86_read_msr( AXK_X86_MSR_APIC );
    bool b_xapic_enabled    = AXK_CHECK_FLAG( apic_msr, 1 << 11 );
    bool b_x2apic_enabled   = AXK_CHECK_FLAG( apic_msr, 1 << 10 );
    bool b_bsp              = AXK_CHECK_FLAG( apic_msr, 1 << 8 );

    if( !b_bsp || b_x2apic_enabled || !b_xapic_enabled ) { return false; }

    // Read ACPI tables
    struct axk_x86_acpi_info_t* ptr_acpi = axk_x86_acpi_get();
    if( ptr_acpi == NULL ) { return false; }

    // Copy in table data we need to keep after tables are released
    this->ioapic_count              = ptr_acpi->ioapic_count;
    this->source_override_count     = ptr_acpi->source_override_count;

    this->ioapic_list           = AXK_CALLOC_TYPE( struct axk_x86_ioapic_info_t, this->ioapic_count );
    this->source_override_list  = AXK_CALLOC_TYPE( struct axk_x86_int_source_override_t, this->source_override_count );

    memcpy( (void*) this->ioapic_list, (void*) ptr_acpi->ioapic_list, sizeof( struct axk_x86_ioapic_info_t ) * this->ioapic_count );
    memcpy( (void*) this->source_override_list, (void*) ptr_acpi->source_override_list, sizeof( struct axk_x86_int_source_override_t ) * this->source_override_count );

    // Ensure APIC is enabled
    axk_x86_write_msr( AXK_X86_MSR_APIC, ptr_acpi->lapic_addr | 0x800UL );

    // Map the address range used by the local APIC so we can access it
    uint64_t lstart_page     = ptr_acpi->lapic_addr / 0x1000UL;
    uint64_t lend_page       = ( ptr_acpi->lapic_addr + 0x3F0UL ) / 0x1000UL;

    uint64_t virt_addr = axk_acquire_shared_address( lstart_page == lend_page ? 1UL : 2UL );

    if( !axk_kmap( lstart_page, virt_addr, AXK_FLAG_PAGEMAP_DISABLE_CACHE ) ) { return false; }
    if( lstart_page != lend_page && !axk_kmap( lend_page, virt_addr + 0x1000UL, AXK_FLAG_PAGEMAP_DISABLE_CACHE ) ) { return false; }

    this->lapic_address = virt_addr + ( ptr_acpi->lapic_addr % 0x1000UL );
    axk_spinlock_init( &( this->ioapic_lock ) );

    // Now we need to map the IOAPIC registers
    uint32_t index = 0;
    for( uint32_t i = 0; i < this->ioapic_count; i++ )
    {
        struct axk_x86_ioapic_info_t* ptr_ioapic = this->ioapic_list + i;

        // Calculate the page(s) this occupies
        uint64_t istart_page = ptr_ioapic->address / 0x1000UL;
        uint64_t iend_page = ( ptr_ioapic->address + 0x14UL ) / 0x1000UL;

        // Allocate some shared address space to map these to
        uint64_t ivirt_addr = axk_acquire_shared_address( istart_page == iend_page ? 1UL : 2UL );
        if( !axk_kmap( istart_page, ivirt_addr, AXK_FLAG_PAGEMAP_DISABLE_CACHE ) ) { return false; }
        if( istart_page != iend_page && !axk_kmap( iend_page, ivirt_addr + 0x1000UL, AXK_FLAG_PAGEMAP_DISABLE_CACHE ) ) { return false; }

        ptr_ioapic->address = ivirt_addr + ( ptr_ioapic->address % 0x1000UL );

        // Read how many inputs this IOAPIC chip has
        ptr_ioapic->interrupt_count = (uint8_t)( axk_x86_xapic_read_ioapic( this, ptr_ioapic->id, IOAPIC_REGISTER_VERSION ) & 0x00FF0000U );
        index++;
    }

    // Disable the legacy external interrupt chips
    if( ptr_acpi->b_legacy_pic )
    {
        axk_x86_xapic_disable_pic( this );
    }

    // Initialize the BSP LAPIC
    axk_x86_xapic_init_lapic( this, ptr_acpi->lapic_nmi_list, ptr_acpi->lapic_nmi_count );

    // Initialize the IOAPIC
    axk_x86_xapic_init_ioapic( this, ptr_acpi->ioapic_nmi_list, ptr_acpi->ioapic_nmi_count );

    // Load the IDT
    axk_x86_load_idt();

    return true;
}


bool xapic_driver_aux_init( struct axk_interrupt_driver_t* this_generic )
{
    // Cast 'this'
    struct axk_x86_xapic_driver_t* this = (struct axk_x86_xapic_driver_t*)( this_generic );

    // Load the IDT
    axk_x86_load_idt_aux();

    return true;
}


void xapic_driver_signal_eoi( struct axk_interrupt_driver_t* this_generic )
{
    axk_x86_xapic_write_lapic( (struct axk_x86_xapic_driver_t*) this_generic, LAPIC_REGISTER_EOI, 0x00 );
}


bool xapic_driver_send_ipi( struct axk_interrupt_driver_t* this_generic, struct axk_interprocessor_interrupt_t* params )
{
    if( params == NULL ) { return false; }

    // Cast 'this'
    struct axk_x86_xapic_driver_t* this = (struct axk_x86_xapic_driver_t*)( this_generic );

    // Validate the request
    bool b_self_target      = false;
    bool b_valid_target     = false;
    uint8_t target_lapic    = 0;

    struct axk_x86_acpi_info_t* ptr_acpi = axk_x86_acpi_get();
    for( uint8_t i = 0; i < ptr_acpi->lapic_count; i++ )
    {
        // Ensure the target processor is valid
        if( ptr_acpi->lapic_list[ i ].processor == params->target_processor )
        {
            target_lapic = ptr_acpi->lapic_list[ i ].id;
            b_valid_target = true;

            if( params->target_processor == axk_get_processor_id() )
            {
                b_self_target = true;
            }

            break;
        }
    }

    if( !b_valid_target ) { return false; }

    // Now we need to start building the request
    uint32_t ipi_parameters;
    switch( params->delivery_mode )
    {
        case AXK_IPI_DELIVERY_MODE_INIT:
        ipi_parameters |= 0b01000000000U;
        break;
        case AXK_IPI_DELIVERY_MODE_START:
        ipi_parameters |= 0b11000000000U;

        // Fallthrough intentional
        default:
        ipi_parameters |= ( 0x000000FFU & params->interrupt_vector );
        break;
    }

    if( !params->b_deassert )
    {
        ipi_parameters |= ( 1U << 14U );
    }

    // Determine if we should be waiting for the IPI to send
    bool b_should_wait = ( params->b_wait_for_receipt && !b_self_target );

    // We need to make sure this thread isnt preempted while were sending the IPI
    uint64_t rflags = axk_disable_interrupts();

    axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_IPI_DESTINATION, target_lapic << 24U );
    axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_IPI_PARAMETERS, ipi_parameters );

    // Wait if needed
    if( b_should_wait )
    {
        do
        {
            __asm__ volatile( "pause" ::: "memory" );
        }
        while( ( axk_x86_xapic_read_lapic( this, LAPIC_REGISTER_IPI_PARAMETERS ) & ( 1U << 12U ) ) != 0x00U );
    }

    // Restore previous interrupt state
    axk_restore_interrupts( rflags );

    return true;
}


bool xapic_driver_set_external_routing( struct axk_interrupt_driver_t* this_generic, uint32_t ext_number, struct axk_external_interrupt_routing_t* routing )
{
    if( routing == NULL ) { return false; }

    // Cast 'this'
    struct axk_x86_xapic_driver_t* this = (struct axk_x86_xapic_driver_t*)( this_generic );

    // We only have 4 bits available to specify a target processor
    if( routing->target_processor > 0b1111 ) 
    {
        axk_terminal_prints( "xAPIC Driver: [Warning] Attempt to route external interrupt to a processor with an out-of-bounds identifier (Available: [0,15])\n" );
        return false;
    }

    // Now we need to find the IOAPIC responsible for this global interrupt number
    struct axk_x86_ioapic_info_t* target_ioapic = NULL;
    for( uint32_t i = 0; i < this->ioapic_count; i++ )
    {
        if( this->ioapic_list[ i ].interrupt_base <= ext_number && 
            ( this->ioapic_list[ i ].interrupt_base + this->ioapic_list[ i ].interrupt_count ) > ext_number )
        {
            target_ioapic = this->ioapic_list + i;
            break;
        }
    }

    if( target_ioapic == NULL )
    {
        axk_terminal_prints( "xAPIC Driver: [Warning] Attempt to set external routing for an out-of-bounds external interrupt vector\n" );
        return false;
    }

    // Determine what register we need to write to, and build values to actually write
    uint32_t reg_addr   = ( ( ext_number - target_ioapic->interrupt_base ) * 0x02U ) + 0x10U;
    uint32_t reg_low    = (uint32_t) routing->local_interrupt;
    uint32_t reg_high   = ( ( routing->target_processor & 0xFFU ) << 24U );

    if( routing->b_low_priority )       { reg_low |= ( 1U << 8U ); }
    if( routing->b_active_low )         { reg_low |= ( 1U << 13U ); }
    if( routing->b_level_triggered )    { reg_low |= ( 1U << 15U ); }
    if( routing->b_masked )             { reg_low |= ( 1U << 16U ); }

    // Ensure the lock is held while writing to the IOAPICs
    // TODO: Seperate lock per IOAPIC? 
    axk_spinlock_acquire( &( this->ioapic_lock ) );

    // Write register values
    axk_x86_xapic_write_ioapic( this, target_ioapic->id, reg_addr, reg_low );
    axk_x86_xapic_write_ioapic( this, target_ioapic->id, reg_addr + 0x01U, reg_high );

    axk_spinlock_release( &( this->ioapic_lock ) );

    return true;
}


bool xapic_driver_get_external_routing( struct axk_interrupt_driver_t* this_generic, uint32_t ext_number, struct axk_external_interrupt_routing_t* out_routing )
{
    if( out_routing == NULL ) { return false; }

    // Cast 'this'
    struct axk_x86_xapic_driver_t* this = (struct axk_x86_xapic_driver_t*)( this_generic );

    // Get the target IOAPIC
    struct axk_x86_ioapic_info_t* target_ioapic = NULL;
    for( uint32_t i = 0; i < this->ioapic_count; i++ )
    {
        if( this->ioapic_list[ i ].interrupt_base <= ext_number && 
            ( this->ioapic_list[ i ].interrupt_base + this->ioapic_list[ i ].interrupt_count ) > ext_number )
        {
            target_ioapic = this->ioapic_list + i;
            break;
        }
    }

    if( target_ioapic == NULL )
    {
        axk_terminal_prints( "xAPIC Driver: [Warning] Attempt to set external routing for an out-of-bounds external interrupt vector\n" );
        return false;
    }

    // Determine what registers to read from
    uint32_t reg_addr   = ( ( ext_number - target_ioapic->interrupt_base ) * 0x02U ) + 0x10U;

    // Acquire IOAPIC lock while reading registers so we arent preempted
    axk_spinlock_acquire( &( this->ioapic_lock ) );

    uint32_t reg_low    = axk_x86_xapic_read_ioapic( this, target_ioapic->id, reg_addr );
    uint32_t reg_high   = axk_x86_xapic_read_ioapic( this, target_ioapic->id, reg_addr + 0x01U );

    axk_spinlock_release( &( this->ioapic_lock ) );

    // Parse the routing info from the IOAPIC register values
    out_routing->local_interrupt    = (uint8_t)( reg_low & 0x000000FFU );
    out_routing->global_interrupt   = ext_number;
    out_routing->b_low_priority     = ( ( reg_low & 0b11100000000U ) == 0b00100000000U );
    out_routing->b_active_low       = AXK_CHECK_FLAG( reg_low, 1U << 13U );
    out_routing->b_level_triggered  = AXK_CHECK_FLAG( reg_low, 1U << 15U );
    out_routing->b_masked           = AXK_CHECK_FLAG( reg_low, 1U << 16U );
    out_routing->target_processor   = (uint32_t)( ( reg_high & 0xFF000000U ) >> 24U );

    return true;
}


uint32_t xapic_driver_get_error( struct axk_interrupt_driver_t* this_generic )
{
    uint64_t rflags = axk_disable_interrupts();
    uint32_t err_code = axk_x86_xapic_read_lapic( (struct axk_x86_xapic_driver_t*) this_generic, LAPIC_REGISTER_ERROR_STATUS );
    axk_restore_interrupts( rflags );

    return err_code;
}


void xapic_driver_clear_error( struct axk_interrupt_driver_t* this_generic )
{
    uint64_t rflags = axk_disable_interrupts();
    axk_x86_xapic_write_lapic( (struct axk_x86_xapic_driver_t*) this_generic, LAPIC_REGISTER_ERROR_STATUS, 0x00U );
    axk_restore_interrupts( rflags );
}


uint32_t xapic_driver_get_ext_int( struct axk_interrupt_driver_t* this_generic, uint8_t bus, uint8_t irq )
{
    // Cast 'this'
    struct axk_x86_xapic_driver_t* this = (struct axk_x86_xapic_driver_t*)( this_generic );

    // Look through external source overrides to determine the global interrupt vector
    for( uint32_t i = 0; i < this->source_override_count; i++ )
    {
        if( this->source_override_list[ i ].bus == bus && this->source_override_list[ i ].irq == irq )
        {
            return this->source_override_list[ i ].global_interrupt;
        }
    }

    // Couldnt find any overrides, so assume its the same vector given
    return irq;
}


struct axk_interrupt_driver_t* axk_x86_create_xapic_driver( void )
{
    // Create driver instance
    struct axk_x86_xapic_driver_t* out_driver = (struct axk_x86_xapic_driver_t*) malloc( sizeof( struct axk_x86_xapic_driver_t) );
    
    // Load function table
    out_driver->func_table.init                     = xapic_driver_init;
    out_driver->func_table.aux_init                 = xapic_driver_aux_init;
    out_driver->func_table.signal_eoi               = xapic_driver_signal_eoi;
    out_driver->func_table.send_ipi                 = xapic_driver_send_ipi;
    out_driver->func_table.set_external_routing     = xapic_driver_set_external_routing;
    out_driver->func_table.get_external_routing     = xapic_driver_get_external_routing;
    out_driver->func_table.get_error                = xapic_driver_get_error;
    out_driver->func_table.clear_error              = xapic_driver_clear_error;
    out_driver->func_table.get_ext_int              = xapic_driver_get_ext_int;

    return (struct axk_interrupt_driver_t*)( out_driver );
}


void axk_x86_xapic_disable_pic( struct axk_x86_xapic_driver_t* this )
{
    axk_x86_outb( PIC1_CMD, ICW1_INIT | ICW1_ICW4 );
    axk_x86_waitio();
    axk_x86_outb( PIC2_CMD, ICW1_INIT | ICW1_ICW4 );
    axk_x86_waitio();
    axk_x86_outb( PIC1_DATA, 0x20 );
    axk_x86_waitio();
    axk_x86_outb( PIC2_DATA, 0x28 );
    axk_x86_waitio();
    axk_x86_outb( PIC1_DATA, 0x04 );
    axk_x86_waitio();
    axk_x86_outb( PIC2_DATA, 0x02 );
    axk_x86_waitio();
    axk_x86_outb( PIC1_DATA, ICW4_8086 );
    axk_x86_waitio();
    axk_x86_outb( PIC2_DATA, ICW4_8086 );
    axk_x86_waitio();
    axk_x86_outb( PIC2_DATA, 0xFF );
    axk_x86_waitio();
    axk_x86_outb( PIC1_DATA, 0xFF );
    axk_x86_waitio();
}


void axk_x86_xapic_init_lapic( struct axk_x86_xapic_driver_t* this, struct axk_x86_lapic_nmi_t* nmi_list, uint32_t nmi_count )
{
    // Write default LINT settings
    axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_LVT_INT0, 0x720 );
    axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_LVT_INT1, 0x402 );

    // Read the MADT entries for this LAPIC
    uint8_t local_id = (uint8_t)( ( axk_x86_xapic_read_lapic( this, LAPIC_REGISTER_ID ) & 0xFF000000U ) >> 24 );
    for( uint32_t i = 0; i < nmi_count; i++ )
    {
        struct axk_x86_lapic_nmi_t* ptr_nmi = nmi_list + i;
        if( ptr_nmi->processor == 0xFF || ptr_nmi->processor == local_id )
        {
            // Read the flags
            bool b_active_low       = AXK_CHECK_FLAG( ptr_nmi->flags, 0x0002U );
            bool b_level_triggered  = AXK_CHECK_FLAG( ptr_nmi->flags, 0x0008U );

            // Determine which LINT (0 or 1) need to be setup as an NMI input
            if( ptr_nmi->lint == 0x00 )
            {
                axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_LVT_INT0,
                    0x402 | ( b_active_low ? ( 1U << 13U ) : 0U ) | ( b_level_triggered ? ( 1U << 15U ) : 0U )
                );
            }
            else
            {
                axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_LVT_INT1,
                    0x402 | ( b_active_low ? ( 1U << 13U ) : 0U ) | ( b_level_triggered ? ( 1U << 15U ) : 0U )
                );
            }
        }
    }

    // CMCI: Direct any machine check interrupts from the LAPIC to vector 0x12
    axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_LVT_CMCI, 0x12 );

    // Errors: Direct any LAPIC errors to interrupt vector 0x30
    axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_LVT_ERROR, (uint32_t) AXK_INT_ERROR );

    // Timer: Setup the initial state, the local timer driver will actually perform all timer functions
    axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_LVT_TIMER, (uint32_t) AXK_INT_LOCAL_TIMER );

    // SIV: Write to this register will direct spurious interrupts and enable the LAPIC
    axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_SPURIOUS_VECTOR, 0x1FFU );
    axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_TASK_PRIORITY, 0x00 );

    // And finally, write to the EOI reigster incase there are pending interrupts
    axk_x86_xapic_write_lapic( this, LAPIC_REGISTER_EOI, 0x00U );
}  


void axk_x86_xapic_init_ioapic( struct axk_x86_xapic_driver_t* this, struct axk_x86_ioapic_nmi_t* nmi_list, uint32_t nmi_count )
{
    // Setup all IOAPICs installed in the system
    for( uint32_t i = 0; i < nmi_count; i++ )
    {
        struct axk_x86_ioapic_nmi_t* ptr_nmi = nmi_list + i;

        struct axk_external_interrupt_routing_t route;

        route.global_interrupt      = ptr_nmi->global_interrupt;
        route.local_interrupt       = AXK_INT_NMI;
        route.b_low_priority        = false;
        route.b_active_low          = AXK_CHECK_FLAG( ptr_nmi->flags, 0x0002U );
        route.b_level_triggered     = AXK_CHECK_FLAG( ptr_nmi->flags, 0x0008U );
        route.b_masked              = false;
        route.target_processor      = 0x00; 

        if( !xapic_driver_set_external_routing( (struct axk_interrupt_driver_t*)this, ptr_nmi->global_interrupt, &route ) )
        {
            axk_terminal_prints( "xAPIC Driver: [Warning] Failed to setup an external NMI!\n" );
        }
    }
}


void axk_x86_xapic_write_ioapic( struct axk_x86_xapic_driver_t* this, uint8_t id, uint8_t reg, uint32_t val )
{
    // Find the base address for this IOAPIC
    uint32_t volatile* ptr_reg = NULL;
    for( uint8_t i = 0; i < this->ioapic_count; i++ )
    {
        if( this->ioapic_list[ i ].id == i )
        {
            ptr_reg = (uint32_t volatile*)this->ioapic_list[ i ].address;
            break;
        }
    }

    if( ptr_reg == NULL )
    {
        axk_terminal_prints( "xAPIC Driver: [ERROR] Attempt to access an invalid IOAPIC\n" );
        return;
    }

    ptr_reg[ 0 ] = ( reg & 0x000000FFU );
    ptr_reg[ 4 ] = val;
}


uint32_t axk_x86_xapic_read_ioapic( struct axk_x86_xapic_driver_t* this, uint8_t id, uint8_t reg )
{
    // Find the base address for this IOAPIC
    uint32_t volatile* ptr_reg = NULL;
    for( uint8_t i = 0; i < this->ioapic_count; i++ )
    {
        if( this->ioapic_list[ i ].id == id )
        {
            ptr_reg = (uint32_t volatile*)this->ioapic_list[ i ].address;
            break;
        }
    }

    if( ptr_reg == NULL )
    {
        axk_terminal_prints( "xAPIC Driver: [ERROR] Attempt to access an invalid IOAPIC\n" );
        return 0U;
    }

    ptr_reg[ 0 ] = ( reg & 0x000000FFU );
    return ptr_reg[ 4 ];
}


void axk_x86_xapic_write_lapic( struct axk_x86_xapic_driver_t* this, uint32_t reg, uint32_t value )
{
    if( reg > 0x3F0 ) 
    { 
        axk_terminal_prints( "xAPIC Driver: [ERROR] Attempt to access invalid local APIC register\n" );
        return;
    }

    *(uint32_t volatile*)( this->lapic_address + (uint64_t)reg ) = value;
}


uint32_t axk_x86_xapic_read_lapic( struct axk_x86_xapic_driver_t* this, uint32_t reg )
{
    if( reg > 0x3F0 ) 
    { 
        axk_terminal_prints( "xAPIC Driver: [ERROR] Attempt to access invalid local APIC register\n" );
        return 0U;
    }

    return *(uint32_t volatile*)( this->lapic_address + (uint64_t)reg );
}


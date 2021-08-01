/*==============================================================
    Axon Kernel - C Entry Point (x86)
    2021, Zachary Berry
    axon/source/arch_x86/entry.c
==============================================================*/
#ifdef _X86_64_

#include "axon/panic.h"
#include "axon/debug_print.h"
#include "axon/memory/page_alloc_private.h"
#include "axon/memory/kmap_private.h"
#include "axon/memory/page_alloc.h"
#include "axon/memory/kmap.h"
#include "axon/memory/kheap_private.h"
#include "axon/memory/kheap.h"
#include "axon/arch.h"
#include "axon/system/interrupts_private.h"
#include "axon/system/timers_private.h"
#include "axon/system/timers.h"
#include "axon/system/time.h"
#include "axon/memory/atomics.h"
#include "axon/system/sysinfo_private.h"
#include "axon/system/sysinfo.h"
#include "axon/arch_x86/acpi_info.h"
#include "axon/arch_x86/util.h"
#include "axon/scheduler/global_scheduler.h"
#include "axon/scheduler/local_scheduler.h"
#include "axon/system/interlink_private.h"
#include "axon/arch_x86/boot_params.h"
#include "axon/boot/boot_params.h"
#include "string.h"
#include "stdlib.h"


/*
    State
*/
static struct axk_atomic_flag_t g_schd_sync;
static struct axk_atomic_flag_t g_topology_sync;
static struct axk_atomic_flag_t g_test_sync;
static struct axk_atomic_uint32_t g_cpu_id;

static bool g_ap_init                                       = false;
static struct axk_cpu_local_storage_t* g_cpu_local_storage  = NULL;

// DEBUG
static uint8_t* g_video_buffer      = NULL;
static size_t g_video_buffer_size   = 0UL;

/*
    ASM Functions/Variables
*/
extern uint64_t axk_get_ap_code_begin( void );
extern uint64_t axk_get_ap_code_size( void );
extern void axk_cleanup_bootstrap( void );
extern uint32_t axk_ap_counter;
extern uint64_t axk_ap_stack;
extern uint64_t axk_ap_wait_flag;

/*
    Forward Decl.
*/
bool axk_x86_parse_topology( void );
void axk_x86_get_core_topology( uint32_t* out_smt, uint32_t* out_core, uint32_t* out_package );
uint32_t axk_x86_get_cache_topology( uint32_t* out_l1_index, uint32_t* out_l2_index, uint32_t* out_l3_index, uint32_t* out_l4_index,
    uint32_t* out_l1_size, uint32_t* out_l2_size, uint32_t* out_l3_size, uint32_t* out_l4_size );
bool axk_start_aux_processors( uint32_t* out_cpu_count );
bool axk_build_system_info_bsp( uint32_t cpu_count );
bool axk_build_system_info_ap( uint32_t id );


void axk_x86_c_bsp_entry( void* ptr_info )
{
    // Parse the boot parameters
    if( !axk_x86_bootparams_parse( ptr_info ) )
    {
        // TODO: Better way to print error?
        return;
    }

    const struct axk_bootparams_framebuffer_t* ptr_buffer = axk_bootparams_get_framebuffer();
    g_video_buffer          = ptr_buffer->buffer;
    g_video_buffer_size     = ptr_buffer->size;

    axk_x86_debug_image();
    axk_halt();

    /*
        Check for required functionality before continuing
    */
    uint32_t eax, ebx, ecx, edx;
    eax = 0x80000001;
    if( !axk_x86_cpuid_s( &eax, &ebx, &ecx, &edx ) || !AXK_CHECK_FLAG( edx, 1 << 29 ) )
    {
        axk_panic( "Kernel: system doesnt support required features! (001)" );
    }

    /*
        Initialize system counters
    */
    axk_counters_init();

    /*
        Initialize page allocator
    */
    if( !axk_pagemgr_init() )
    {
        axk_panic( "Kernel: failed to initialize page" );
    }

    /*
        Initialize kernel memory map system
    */
    if( !axk_mapmgr_init() )
    {
        axk_panic( "Kernel: failed to initialize kernel memory map" );
    }

    /*
        Initialize kernel heap
    */
    if( !axk_kheap_init() )
    {
        axk_panic( "Kernel: failed to initialize kernel heap" );
    }

    /*
        Initialize Sysinfo
    */
    if( !axk_sysinfo_init() )
    {
        axk_panic( "Kernel: failed to load sysinfo" );
    }

    /*
        Initialize ACPI tables (X86 ONLY)
    */
    if( !axk_x86_acpi_parse() )
    {
        axk_panic( "Kernel: failed to parse ACPI tables" );
    }

    /*
        Initialize interrupt driver
    */
    if( !axk_interrupts_init() )
    {
        axk_panic( "Kernel: failed to initialize interrupt driver" );
    }

    /*
        Allocate CPU-Local storage for all processors
    */
    g_cpu_local_storage = (struct axk_cpu_local_storage_t*) calloc( axk_x86_acpi_get()->lapic_count, sizeof( struct axk_cpu_local_storage_t ) );

    /*
        Check for ACPI SRAT information for the BSP
        Then we fill out our local processor storage
    */
    struct axk_x86_srat_cpu_t* srat_bsp     = NULL;
    struct axk_x86_acpi_info_t* ptr_acpi    = axk_x86_acpi_get();
    bool b_x2mode                           = axk_interrupts_get_type() == INTERRUPT_DRIVER_X86_X2APIC;
    uint32_t bsp_lapic_id                   = axk_interrupts_cpu_id();

    for( uint32_t i = 0; i < ptr_acpi->srat_cpu_count; i++ )
    {
        if( b_x2mode )
        {
            if( ptr_acpi->srat_cpu_list[ i ].x2apic_lapic == bsp_lapic_id )
            {
                srat_bsp = ptr_acpi->srat_cpu_list + i;
                break;
            }
        }
        else
        {
            if( ptr_acpi->srat_cpu_list[ i ].xapic_lapic == bsp_lapic_id )
            {
                srat_bsp = ptr_acpi->srat_cpu_list + i;
                break;
            }
        }
    }

    g_cpu_local_storage[ 0 ].this_address       = (void*)( g_cpu_local_storage );
    g_cpu_local_storage[ 0 ].os_identifier      = 0U;
    g_cpu_local_storage[ 0 ].arch_identifier    = axk_interrupts_cpu_id();
    g_cpu_local_storage[ 0 ].domain             = srat_bsp == NULL ? 0U : srat_bsp->domain;
    g_cpu_local_storage[ 0 ].clock_domain       = srat_bsp == NULL ? 0U : srat_bsp->clock_domain;

    axk_x86_write_gs( (uint64_t) g_cpu_local_storage );
    axk_atomic_store_uint32( &g_cpu_id, 1U, MEMORY_ORDER_SEQ_CST );

    /*
        Load timer system
    */
    if( !axk_timers_init() )
    {
        axk_panic( "Kernel: failed to initialize timer drivers" );
    }

    /*
        Start the Aux Processors
    */
    axk_atomic_clear_flag( &g_schd_sync, MEMORY_ORDER_RELAXED );
    axk_atomic_clear_flag( &g_topology_sync, MEMORY_ORDER_RELAXED );

    uint32_t proc_count;
    if( !axk_start_aux_processors( &proc_count ) )
    {
        axk_panic( "Kernel: failed to start other processors" );
    }

    /*
        Setup System Info
    */
    if( !axk_build_system_info_bsp( proc_count ) )
    {
        axk_panic( "Kernel: failed to build system info" );
    }

    axk_atomic_set_flag( &g_topology_sync, true, MEMORY_ORDER_SEQ_CST );

    /*
        Initialize Interlink System
    */
    if( !axk_interlink_init() )
    {
        axk_panic( "Kernel: failed to initialize Interlink" );
    }

    /*
        Syncronize System Clock
    */
    if( !axk_timers_bsp_sync() )
    {
        axk_panic( "Kernel: failed to syncronize system clock" );
    }

    /*
        Initialize global scheduler
    */
    if( !axk_scheduler_init_global() )
    {
        axk_panic( "Kernel: failed to init global scheduler" );
    }

    /*
        Syncronize processors before creating local schedulers
    */
    axk_atomic_clear_flag( &g_test_sync, MEMORY_ORDER_SEQ_CST );
    axk_atomic_set_flag( &g_schd_sync, true, MEMORY_ORDER_SEQ_CST );
    if( !axk_scheduler_init_local() )
    {
        axk_panic( "Kernel: failed to init local scheduler" );
    }
    
    axk_atomic_set_flag( &g_test_sync, true, MEMORY_ORDER_SEQ_CST );


    while( 1 ) { __asm__( "hlt" ); }

}

void axk_c_main_ap( void )
{
    // Acquire a OS assigned CPU identifier so we can access our cpu-local storage
    uint32_t cpu_id = axk_atomic_fetch_add_uint32( &g_cpu_id, 1U, MEMORY_ORDER_SEQ_CST );

    // Find the local processor in the SRAT list if present
    struct axk_x86_srat_cpu_t* srat_ap      = NULL;
    struct axk_x86_acpi_info_t* ptr_acpi    = axk_x86_acpi_get();
    bool b_x2mode                           = axk_interrupts_get_type() == INTERRUPT_DRIVER_X86_X2APIC;
    uint32_t ap_lapic_id                    = axk_interrupts_cpu_id();

    for( uint32_t i = 0; i < ptr_acpi->srat_cpu_count; i++ )
    {
        if( b_x2mode )
        {
            if( ptr_acpi->srat_cpu_list[ i ].x2apic_lapic == ap_lapic_id )
            {
                srat_ap = ptr_acpi->srat_cpu_list + i;
                break;
            }
        }
        else
        {
            if( ptr_acpi->srat_cpu_list[ i ].xapic_lapic == ap_lapic_id )
            {
                srat_ap = ptr_acpi->srat_cpu_list + i;
                break;
            }
        }
    }

    g_cpu_local_storage[ cpu_id ].this_address      = (void*)( g_cpu_local_storage + cpu_id );
    g_cpu_local_storage[ cpu_id ].os_identifier     = cpu_id;
    g_cpu_local_storage[ cpu_id ].arch_identifier   = axk_interrupts_cpu_id();
    g_cpu_local_storage[ cpu_id ].domain            = srat_ap == NULL ? 0U : srat_ap->domain;
    g_cpu_local_storage[ cpu_id ].clock_domain      = srat_ap == NULL ? 0U : srat_ap->clock_domain;

    axk_x86_write_gs( (uint64_t)( g_cpu_local_storage + cpu_id ) );

    /*
        Initialize Interrupts
    */
    if( !axk_interrupts_init_aux() )
    {
        axk_panic( "Kernel: failed to initialize interrupts on aux processor" );
    }

    /*
        Syncronize with BSP
    */
    while( !axk_atomic_test_flag( &g_topology_sync, MEMORY_ORDER_SEQ_CST ) ) { __asm__( "pause" ); }

    /*
        Setup System Info
    */
    if( !axk_build_system_info_ap( cpu_id ) )
    {
        axk_panic( "Kernel: failed to build system info" );
    }

    /*
        Syncronize System Clock
    */
    if( !axk_timers_ap_sync() )
    {
        axk_panic( "Kernel: failed to syncronize system clock on aux processor" );
    }

    /*
        Syncronize, then create our local scheduler
    */
    while( !axk_atomic_test_flag( &g_schd_sync, MEMORY_ORDER_SEQ_CST ) ) { __asm__( "pause" ); }

    if( !axk_scheduler_init_local() )
    {
        axk_panic( "Kernel: fialed to init local scheduler" );
    }

    while( !axk_atomic_test_flag( &g_test_sync, MEMORY_ORDER_SEQ_CST ) ) { __asm__( "pause" ); }

    while( true ) { __asm__( "pause" ); }

    // We cant actually return from this function, there is no return address on the stack
    axk_halt();
}


bool axk_start_aux_processors( uint32_t* out_cpu_count )
{
    // Ensure this hasnt already been called
    if( g_ap_init || out_cpu_count == NULL ) { return false; }
    g_ap_init = true;

    struct axk_x86_acpi_info_t* ptr_acpi = axk_x86_acpi_get();
    if( ptr_acpi == NULL ) { return false; }

    uint32_t cpu_count  = ptr_acpi->lapic_count;
    uint32_t bsp_id     = ptr_acpi->bsp_id;
    ( *out_cpu_count )     = 1;

    if( cpu_count <= 1 )
    {
        return true;
    }

    uint64_t init_addr      = AXK_AP_INIT_ADDRESS + AXK_KERNEL_VA_PHYSICAL;
    uint64_t code_begin     = axk_get_ap_code_begin();
    uint64_t code_size      = axk_get_ap_code_size();

    // Ensure the init code isnt larger than a single page
    if( code_size > 0x1000UL ) 
    {
        axk_panic( "Boot (x86): Aux Processor init code is larger than one page?" );
    }

    // Copy the code to the physical memory location in low memory
    memcpy( (void*) init_addr, (void*) code_begin, code_size );

    // Setup some state
    axk_ap_wait_flag    = 0;
    axk_ap_counter      = 1;

    // Loop through each processor and start it
    for( uint32_t i = 0; i < ptr_acpi->lapic_count; i++ )
    {
        struct axk_x86_lapic_info_t* ptr_info = ptr_acpi->lapic_list + i;
        if( ptr_info->id == bsp_id ) { continue; }

        // Send INIT signal
        struct axk_interprocessor_interrupt_t init;

        init.target_processor       = (uint32_t) ptr_info->id;
        init.interrupt_vector       = 0;
        init.delivery_mode          = AXK_IPI_DELIVERY_MODE_INIT;
        init.b_deassert             = false;
        init.b_wait_for_receipt     = true;

        axk_interrupts_clear_error();
        if( !axk_interrupts_send_ipi( &init ) )
        {
            // ISSUE: We cant convert to proper LAPIC identifiers!
            return false;
        }

        // TODO: Check for error
        uint32_t init_err = axk_interrupts_get_error();

        // Wait for 10ms
        axk_delay( 10000000UL );

        // Allocate a kernel stack for this processor
        axk_ap_stack = (uint64_t) malloc( AXK_KERNEL_STACK_SIZE ) + AXK_KERNEL_STACK_SIZE;
        uint32_t prev_counter = axk_ap_counter;

        // Send SIPI signal
        init.target_processor       = (uint32_t) ptr_info->id;
        init.interrupt_vector       = 0x08;
        init.delivery_mode          = AXK_IPI_DELIVERY_MODE_START;
        init.b_deassert             = false;
        init.b_wait_for_receipt     = true;

        axk_interrupts_clear_error();
        if( !axk_interrupts_send_ipi( &init ) )
        {
            return false;
        }

        // TODO: Check for errors
        init_err = axk_interrupts_get_error();

        // Wait for 2ms and check if the processor started up as expected
        axk_delay( 2000000UL );

        if( axk_ap_counter == prev_counter )
        {
            // Try one more time to start the processor
            axk_interrupts_clear_error();
            if( !axk_interrupts_send_ipi( &init ) )
            {
                return false;
            }

            // TODO: Check for error
            init_err = axk_interrupts_get_error();
            
            // Wait for a full second, and check again
            axk_delay( 1000000000UL );

            if( axk_ap_counter == prev_counter )
            {
                return false;
            }
        }

        // Processor was started successfully
        ( *out_cpu_count )++;
    }

    // Print debug message
    axk_terminal_lock();
    axk_terminal_prints( "Boot: Started all available processors, there are now " );
    axk_terminal_printu32( *out_cpu_count );
    axk_terminal_prints( " processors running\n" );
    axk_terminal_unlock();

    // Now, we need to unmap the low area of memory 
    //axk_cleanup_bootstrap();

    // And now, release the processors
    axk_ap_wait_flag = 1;
    return true;
}


bool axk_build_system_info_bsp( uint32_t cpu_count )
{
    if( !axk_x86_parse_topology() ) { return false; }

    struct axk_sysinfo_general_t general_info;
    struct axk_sysinfo_processor_t bsp_info;

    axk_x86_get_core_topology( &bsp_info.smt_id, &bsp_info.core_id, &bsp_info.package_id );
    general_info.cache_count = axk_x86_get_cache_topology( &bsp_info.cache_l1_id, &bsp_info.cache_l2_id, &bsp_info.cache_l3_id, &bsp_info.cache_l4_id,
        &bsp_info.cache_l1_size, &bsp_info.cache_l2_size, &bsp_info.cache_l3_size, &bsp_info.cache_l4_size );

    general_info.cpu_count      = cpu_count;
    general_info.total_memory   = axk_pagemgr_get_physmem();
    general_info.bsp_id         = 0;

    axk_sysinfo_write( AXK_SYSINFO_GENERAL, 0, (void*) &general_info, sizeof( general_info ) );

    bsp_info.identifier     = 0;
    bsp_info.type           = AXK_PROCESSOR_TYPE_NORMAL;
    bsp_info.domain         = g_cpu_local_storage[ 0 ].domain;
    bsp_info.clock_domain   = g_cpu_local_storage[ 0 ].clock_domain;

    axk_sysinfo_write( AXK_SYSINFO_PROCESSOR, general_info.bsp_id, (void*) &bsp_info, sizeof( bsp_info ) );

    // DEBUG
    axk_terminal_lock();
    axk_terminal_prints( "====> CPU (OSID: 0)  Domain: " );
    axk_terminal_printu32( bsp_info.domain );
    axk_terminal_prints( " CDM: " );
    axk_terminal_printu32( bsp_info.clock_domain );
    axk_terminal_prints( " Pkg: " );
    axk_terminal_printu32( bsp_info.package_id );
    axk_terminal_prints( " Core: " );
    axk_terminal_printu32( bsp_info.core_id );
    axk_terminal_prints( " SMT: " );
    axk_terminal_printu32( bsp_info.smt_id );
    axk_terminal_printnl();
    axk_terminal_printtab();
    axk_terminal_prints( " L1: " );
    axk_terminal_printu32( bsp_info.cache_l1_id );
    axk_terminal_prints( " (" );
    axk_terminal_printu32( bsp_info.cache_l1_size / 1024 );
    axk_terminal_prints( "KB) L2: " );
    axk_terminal_printu32( bsp_info.cache_l2_id );
    axk_terminal_prints( " (" );
    axk_terminal_printu32( bsp_info.cache_l2_size / 1024 );
    axk_terminal_prints( "KB) L3: " );
    axk_terminal_printu32( bsp_info.cache_l3_id );
    axk_terminal_prints( " (" );
    axk_terminal_printu32( bsp_info.cache_l3_size / 1024 );
    axk_terminal_prints( "KB)\n" );
    axk_terminal_unlock(); 

    return true;
}


bool axk_build_system_info_ap( uint32_t cpu_id )
{
    struct axk_sysinfo_processor_t ap_info;
    axk_x86_get_core_topology( &ap_info.smt_id, &ap_info.core_id, &ap_info.package_id );
    axk_x86_get_cache_topology( &ap_info.cache_l1_id, &ap_info.cache_l2_id, &ap_info.cache_l3_id, &ap_info.cache_l4_id,
        &ap_info.cache_l1_size, &ap_info.cache_l2_size, &ap_info.cache_l3_size, &ap_info.cache_l4_size );

    ap_info.identifier      = axk_get_cpu_id();
    ap_info.type            = AXK_PROCESSOR_TYPE_NORMAL;
    ap_info.domain          = g_cpu_local_storage[ cpu_id ].domain;
    ap_info.clock_domain    = g_cpu_local_storage[ cpu_id ].clock_domain;

    axk_sysinfo_write( AXK_SYSINFO_PROCESSOR, ap_info.identifier, (void*) &ap_info, sizeof( ap_info ) );

    // DEBUG
    axk_terminal_lock();
    axk_terminal_prints( "====> CPU (OSID: " );
    axk_terminal_printu32( ap_info.identifier );
    axk_terminal_prints( ")  Domain: " );
    axk_terminal_printu32( ap_info.domain );
    axk_terminal_prints( " CDM: " );
    axk_terminal_printu32( ap_info.clock_domain );
    axk_terminal_prints( " Pkg: " );
    axk_terminal_printu32( ap_info.package_id );
    axk_terminal_prints( " Core: " );
    axk_terminal_printu32( ap_info.core_id );
    axk_terminal_prints( " SMT: " );
    axk_terminal_printu32( ap_info.smt_id );
    axk_terminal_printnl();
    axk_terminal_printtab();
    axk_terminal_prints( " L1: " );
    axk_terminal_printu32( ap_info.cache_l1_id );
    axk_terminal_prints( " (" );
    axk_terminal_printu32( ap_info.cache_l1_size / 1024 );
    axk_terminal_prints( "KB) L2: " );
    axk_terminal_printu32( ap_info.cache_l2_id );
    axk_terminal_prints( " (" );
    axk_terminal_printu32( ap_info.cache_l2_size / 1024 );
    axk_terminal_prints( "KB) L3: " );
    axk_terminal_printu32( ap_info.cache_l3_id );
    axk_terminal_prints( " (" );
    axk_terminal_printu32( ap_info.cache_l3_size / 1024 );
    axk_terminal_prints( "KB)\n" );
    axk_terminal_unlock(); 

    return true;
}


struct axk_cpu_local_storage_t* axk_get_cpu_local_storage( void )
{
    return (struct axk_cpu_local_storage_t*)( axk_x86_read_gs() );
}


bool axk_x86_convert_cpu_id( uint32_t os_id, uint32_t* out_id )
{
    // Check CPU-Local storage bounds for error
    uint32_t max_id = axk_atomic_load_uint32( &g_cpu_id, MEMORY_ORDER_RELAXED );
    if( os_id >= max_id )
    {
        return false;
    }

    *out_id = g_cpu_local_storage[ os_id ].arch_identifier;
    return true;
}


void axk_x86_debug_image( void )
{
    for( size_t i = 0; i < g_video_buffer_size; i++ )
    {
        g_video_buffer[ i ] = 0xFF;
    }
}


#endif
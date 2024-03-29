/*==============================================================
    Axon Kernel - ACPI Info (x86)
    2021, Zachary Berry
    axon/source/arch_x86/acpi_info.c
==============================================================*/

#ifdef _X86_64_
#include "axon/arch_x86/acpi_info.h"
#include "axon/arch_x86/boot_params.h"
#include "axon/arch_x86/util.h"
#include "axon/arch.h"
#include "axon/boot/basic_out.h"
#include "axon/panic.h"
#include "axon/library/vector.h"
#include "string.h"
#include "stdlib.h"

/*
    Constants
*/
#define ACPI_ENTRY_LAPIC                0x00
#define ACPI_ENTRY_IOAPIC               0x01
#define ACPI_ENTRY_INT_SOURCE_OVERRIDE  0x02
#define ACPI_ENTRY_IOAPIC_NMI           0x03
#define ACPI_ENTRY_LAPIC_NMI            0x04
#define ACPI_ENTRY_LAPIC_ADDRESS        0x05
#define ACPI_ENTRY_IOSAPIC              0x06
#define ACPI_ENTRY_LSAPIC               0x07
#define ACPI_ENTRY_PLATFORM_INTS        0x08
#define ACPI_ENTRY_X2_LAPIC             0x09

#define SRAT_ENTRY_PROCESSOR            0x00
#define SRAT_ENTRY_MEMORY               0x01
#define SRAT_ENTRY_PROCESSOR_X2APIC     0x02

/*
    State
*/
static bool g_init;
static struct axk_x86_acpi_info_t g_acpi;

/*
    Function Implementations
*/

bool parse_cpuid( void )
{
    // Get the manufacturer from CPUID
    // Also, ensure the required CPUID leafs are present
    uint32_t eax, ebx, ecx, edx;
    eax = 0U; ebx = 0; ecx = 0; edx = 0;
    axk_x86_cpuid( &eax, &ebx, &ecx, &edx );

    if( eax < 0x0B ) 
    {
        axk_panic( "x86: CPUID doesnt support the required functionality needed! This system is not supported" );
    }

    memcpy( (void*) g_acpi.cpu_vendor, (void*) &ebx, 4 );
    memcpy( (void*) g_acpi.cpu_vendor + 4, (void*) &edx, 4 );
    memcpy( (void*) g_acpi.cpu_vendor + 8, (void*) &ecx, 4 );

    eax = 1; ebx = 0; ecx = 0; edx = 0;
    axk_x86_cpuid( &eax, &ebx, &ecx, &edx );

    // Check if MP isnt supported
    if( ( edx & ( 1U << 28 ) ) == 0U )
    {
        g_acpi.cpu_count    = 1;
        g_acpi.bsp_id       = g_acpi.lapic_list[ 0 ].processor;

        return true;
    }

    // We need to determine the APIC ID of the bootstrap processor
    g_acpi.cpu_count = g_acpi.lapic_count;
    
    eax = 0x0B; ebx = 0x00; ecx = 0x00; edx = 0x00;
    if( !axk_x86_cpuid_s( &eax, &ebx, &ecx, &edx ) )
    {
        //axk_panic( "ACPI: CPUID doesnt support APIC info leaf" );
        eax = 0x01; ebx = 0x00; ecx = 0x00; edx = 0x00;
        axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
        edx = ( ( ebx & 0xFF000000U ) >> 24 );
    }

    bool b_found_bsp = false;
    for( uint32_t i = 0; i < g_acpi.lapic_count; i++ )
    {
        if( (uint32_t)g_acpi.lapic_list[ i ].processor == edx )
        {
            g_acpi.bsp_id = g_acpi.lapic_list[ i ].id;
            b_found_bsp = true;
            break;
        }
    }

    if( !b_found_bsp )
    {
        axk_basicout_prints( "ACPI: [Warning] Failed to find BSP APIC ID, defaulting to '0'\n" );
        g_acpi.bsp_id = 0;
    }

    // Now were going to finish parsing system topology

    return true;
}


bool acpi_validate( uint64_t address, const char* name )
{
    struct axk_x86_acpi_header_t* ptr_header = (struct axk_x86_acpi_header_t*)( address );

    uint8_t checksum = 0;
    for( uint64_t i = 0; i < ptr_header->length; i++ )
    {
        checksum += *( (uint8_t*)( address + i ) );
    } 

    return( ( checksum == 0 ) && memcmp( (void*) address, (void*) name, 4 ) == 0 );
}


bool parse_madt( uint64_t address )
{
    struct axk_x86_acpi_header_t* ptr_header = (struct axk_x86_acpi_header_t*)( address );

    // Start reading some values from the table...
    g_acpi.lapic_addr   = (uint64_t)( *( (uint32_t*)( address + 0x24UL ) ) );
    uint32_t flags      = *( (uint32_t*)( address + 0x28UL ) );

    uint8_t* list_begin     = (uint8_t*)( address + 0x2CUL );
    uint8_t* list_end       = (uint8_t*)( address + ptr_header->length );
    uint8_t* list_pos       = list_begin;

    // We need to go through, and count how many of each type of entries there are
    // This way, we can allocate the needed amount of memory before reading these tables in
    uint32_t lapic_count        = 0;
    uint32_t ioapic_count       = 0;
    uint32_t override_count     = 0;
    uint32_t ioapic_nmi_count   = 0;
    uint32_t lapic_nmi_count    = 0;
    
    while( list_pos < list_end )
    {
        uint8_t entry_type      = *list_pos;
        uint8_t entry_length    = *( list_pos + 0x01UL );

        switch( entry_type )
        {
            case ACPI_ENTRY_LAPIC:
            lapic_count++;
            break;

            case ACPI_ENTRY_IOAPIC:
            ioapic_count++;
            break;

            case ACPI_ENTRY_INT_SOURCE_OVERRIDE:
            override_count++;
            break;

            case ACPI_ENTRY_IOAPIC_NMI:
            ioapic_nmi_count++;
            break;

            case ACPI_ENTRY_LAPIC_NMI:
            lapic_nmi_count++;
            break;
        }

        list_pos += entry_length;
    }

    // Ensure there is an LAPIC and at least one IOAPIC
    if( lapic_count == 0 || ioapic_count == 0 )
    {
        axk_panic( "ACPI: failed to parse, missing APIC info!" );
    }

    // Allocate memory to hold the data we want to read in
    g_acpi.lapic_count              = lapic_count;
    g_acpi.ioapic_count             = ioapic_count;
    g_acpi.lapic_nmi_count          = lapic_nmi_count;
    g_acpi.ioapic_nmi_count         = ioapic_nmi_count;
    g_acpi.source_override_count    = override_count;
    g_acpi.b_legacy_pic             = AXK_CHECK_FLAG( flags, 0x01 );

    g_acpi.lapic_list               = lapic_count > 0 ? AXK_CALLOC_TYPE( struct axk_x86_lapic_info_t, lapic_count ) : NULL;
    g_acpi.ioapic_list              = ioapic_count > 0 ? AXK_CALLOC_TYPE( struct axk_x86_ioapic_info_t, ioapic_count ) : NULL;
    g_acpi.lapic_nmi_list           = lapic_nmi_count > 0 ? AXK_CALLOC_TYPE( struct axk_x86_lapic_nmi_t, lapic_nmi_count ) : NULL;
    g_acpi.ioapic_nmi_list          = ioapic_nmi_count > 0 ? AXK_CALLOC_TYPE( struct axk_x86_ioapic_nmi_t, ioapic_nmi_count ) : NULL;
    g_acpi.source_override_list     = override_count > 0 ? AXK_CALLOC_TYPE( struct axk_x86_int_source_override_t, override_count ) : NULL;

    // Loop through the list again, and actually read the data in this time
    list_pos = list_begin;
    while( list_pos < list_end )
    {
        uint8_t entry_type      = *list_pos;
        uint8_t entry_length    = *( list_pos + 0x01UL );

        switch( entry_type )
        {
            case ACPI_ENTRY_LAPIC:
            {
                struct axk_x86_lapic_info_t* ptr_info = g_acpi.lapic_list + ( --lapic_count );

                ptr_info->processor     = *( list_pos + 0x02UL );
                ptr_info->id            = *( list_pos + 0x03UL );
                ptr_info->flags         = *( (uint32_t*)( list_pos + 0x04UL ) );

                break;
            }
            case ACPI_ENTRY_IOAPIC:
            {
                struct axk_x86_ioapic_info_t* ptr_info = g_acpi.ioapic_list + ( --ioapic_count );

                ptr_info->id                = *( list_pos + 0x02UL );
                ptr_info->address           = *( (uint32_t*)( list_pos + 0x04UL ) );
                ptr_info->interrupt_base    = *( (uint32_t*)( list_pos + 0x08UL ) );

                break;
            }
            case ACPI_ENTRY_INT_SOURCE_OVERRIDE:
            {
                struct axk_x86_int_source_override_t* ptr_info = g_acpi.source_override_list + ( --override_count );

                ptr_info->bus               = *( list_pos + 0x02UL );
                ptr_info->irq               = *( list_pos + 0x03UL );
                ptr_info->global_interrupt  = *( (uint32_t*)( list_pos + 0x04UL ) );
                ptr_info->flags             = *( (uint16_t*)( list_pos + 0x08UL ) );
                break;
            }
            case ACPI_ENTRY_IOAPIC_NMI:
            {
                struct axk_x86_ioapic_nmi_t* ptr_info = g_acpi.ioapic_nmi_list + ( --ioapic_nmi_count );

                ptr_info->source            = *( list_pos + 0x02UL );
                ptr_info->flags             = *( (uint16_t*)( list_pos + 0x04UL ) );
                ptr_info->global_interrupt  = *( (uint32_t*)( list_pos + 0x06UL ) );
                break;
            }
            case ACPI_ENTRY_LAPIC_NMI:
            {
                struct axk_x86_lapic_nmi_t* ptr_info = g_acpi.lapic_nmi_list + ( --lapic_nmi_count );

                ptr_info->processor         = *( list_pos + 0x02UL );
                ptr_info->flags             = *( (uint16_t*)( list_pos + 0x03UL ) );
                ptr_info->lint              = *( list_pos + 0x05UL );
                break;
            }
            case ACPI_ENTRY_LAPIC_ADDRESS:
            {
                g_acpi.lapic_addr = *( (uint64_t*)( list_pos + 0x04UL ) );
                break;
            }
            
            case ACPI_ENTRY_X2_LAPIC:
            {
                break;
            }

            case ACPI_ENTRY_IOSAPIC:
            {
                axk_basicout_prints( "[DEBUG] ACPI: Found an I/O SAPIC Entry\n" );
                break;
            }

            case ACPI_ENTRY_LSAPIC:
            {
                axk_basicout_prints( "[DEBUG] ACPI: Found a Local SAPIC Entry\n" );
                break;
            }

            case ACPI_ENTRY_PLATFORM_INTS:
            {
                axk_basicout_prints( "[DEBUG] ACPI: Found Platform Interrupt Source Entry\n" );
                break;
            }
        }

        list_pos += entry_length;
    }

    return true;
}


bool parse_fadt( uint64_t address )
{
    g_acpi.fadt = (struct axk_x86_acpi_fadt_t*)( address + sizeof( struct axk_x86_acpi_header_t ) );
    return true;
}


bool parse_ssdt( uint64_t address )
{
    // TODO: We dont actually need anything from this table yet
    return true;
}


bool parse_srat( uint64_t address )
{
    struct axk_x86_acpi_header_t* ptr_header = (struct axk_x86_acpi_header_t*)( address );

    // First, were going to count how many of each entries there are so we can allocate storage
    uint32_t cpu_num        = 0U;
    uint32_t mem_num        = 0U;
    uint32_t cpu_x2_num     = 0U;
    uint8_t* ptr_pos        = (uint8_t*)( address + 0x30UL );
    uint8_t* ptr_end        = (uint8_t*)( address + (uint64_t)ptr_header->length );

    while( ptr_pos < ptr_end )
    {
        // Read the next entry type
        switch( *ptr_pos )
        {
            case SRAT_ENTRY_PROCESSOR:
            
            // Check if this entry is enabled
            if( AXK_CHECK_FLAG( AXK_READ_UINT8( ptr_pos + 4UL ), 0b1 ) )
            {
                cpu_num++;
            }
            break;

            case SRAT_ENTRY_MEMORY:

            // Check if this entry is enabled
            if( AXK_CHECK_FLAG( AXK_READ_UINT8( ptr_pos + 28UL ), 0b1 ) )
            {
                mem_num++;
            }
            break;

            case SRAT_ENTRY_PROCESSOR_X2APIC:

            // Check if this entry is enabled
            if( AXK_CHECK_FLAG( AXK_READ_UINT8( ptr_pos + 12UL ), 0b1 ) )
            {
                cpu_x2_num++;
            }
            break;

            default:
            break;
        }
    }

    // Allocate storage in global ACPI structure
    g_acpi.srat_cpu_count      = cpu_num > cpu_x2_num ? cpu_num : cpu_x2_num;
    g_acpi.srat_memory_count   = mem_num;
    g_acpi.srat_cpu_list       = g_acpi.srat_cpu_count > 0U ? (struct axk_x86_srat_cpu_t*) calloc( g_acpi.srat_cpu_count, sizeof( struct axk_x86_srat_cpu_t ) ) : NULL;
    g_acpi.srat_memory_list    = g_acpi.srat_memory_count > 0U ? (struct axk_x86_srat_memory_t*) calloc( g_acpi.srat_memory_count, sizeof( struct axk_x86_srat_memory_t ) ) : NULL;

    // If there are no structures to fill, then were finished
    if( g_acpi.srat_cpu_list == NULL && g_acpi.srat_memory_list == NULL ) { return true; }
    uint32_t cpu_counter = 0U;
    uint32_t mem_counter = 0U;

    // Fill the newly allocate structures
    ptr_pos = (uint8_t*)( address + 0x30UL );
    while( ptr_pos < ptr_end )
    {
        // Read the entry type
        switch( *ptr_pos )
        {
            case SRAT_ENTRY_PROCESSOR:
            {
                // Ensure this entry is enabled
                if( !AXK_CHECK_FLAG( AXK_READ_UINT8( ptr_pos + 4UL ), 0b1 ) ) { continue; }

                // Check if theres an existing processor in the list, because an x2apic entry was read first
                uint8_t id      = AXK_READ_UINT8( ptr_pos + 3UL );
                bool b_found    = false;

                for( uint32_t i = 0; i < cpu_counter; i++ )
                {
                    if( g_acpi.srat_cpu_list[ i ].x2apic_lapic == (uint32_t)id )
                    {
                        // Found an existing entry for this processor, so just update the 8-bit identifier
                        g_acpi.srat_cpu_list[ i ].xapic_lapic = id;
                        b_found = true;
                        break;
                    }
                }

                if( !b_found )
                {
                    // Create a new entry
                    struct axk_x86_srat_cpu_t* new_entry = g_acpi.srat_cpu_list + ( cpu_counter++ );

                    // Read the domain identifier
                    uint8_t domain[ 4 ];
                    domain[ 0 ] = AXK_READ_UINT8( ptr_pos + 2UL );
                    domain[ 1 ] = AXK_READ_UINT8( ptr_pos + 9UL );
                    domain[ 2 ] = AXK_READ_UINT8( ptr_pos + 10UL );
                    domain[ 3 ] = AXK_READ_UINT8( ptr_pos + 11UL );

                    new_entry->domain           = ( (uint32_t)( domain[ 3 ] ) << 24 ) | ( (uint32_t)( domain[ 2 ] ) << 16 ) | ( (uint32_t)( domain[ 1 ] ) << 8 ) | (uint32_t)( domain[ 0 ] );
                    new_entry->xapic_lapic      = id;
                    new_entry->clock_domain     = AXK_READ_UINT32( ptr_pos + 12UL );
                }
                break;
            }
            case SRAT_ENTRY_MEMORY:
            {
                // Ensure this entry is enabled
                if( !AXK_CHECK_FLAG( AXK_READ_UINT8( ptr_pos + 28UL ), 0b1 ) ) { continue; }

                struct axk_x86_srat_memory_t* new_entry = g_acpi.srat_memory_list + ( mem_counter++ );
                new_entry->domain = AXK_READ_UINT32( ptr_pos + 2UL );
                
                // Read other fields from the table
                uint32_t base_low       = AXK_READ_UINT32( ptr_pos + 8UL );
                uint32_t base_high      = AXK_READ_UINT32( ptr_pos + 12UL );
                uint32_t length_low     = AXK_READ_UINT32( ptr_pos + 16UL );
                uint32_t length_high    = AXK_READ_UINT32( ptr_pos + 20UL );
                uint32_t flags          = AXK_READ_UINT32( ptr_pos + 28UL );

                // Write into our structure
                new_entry->base_address     = ( (uint64_t)( base_high ) << 32 ) | (uint64_t)( base_low );
                new_entry->length           = ( (uint64_t)( length_high ) << 32 ) | (uint64_t)( length_low );
                new_entry->b_hotplug        = AXK_CHECK_FLAG( flags, 0b10 );
                new_entry->b_nonvolatile    = AXK_CHECK_FLAG( flags, 0b100 );
                break;
            }
            case SRAT_ENTRY_PROCESSOR_X2APIC:
            {
                // Ensure this entry is enabled
                if( !AXK_CHECK_FLAG( AXK_READ_UINT8( ptr_pos + 12UL ), 0b1 ) ) { continue; }

                // Check if theres an existing processor in the list
                uint32_t id = AXK_READ_UINT32( ptr_pos + 8UL );
                bool b_found    = false;

                if( id <= 0xFFU )
                {
                    for( uint32_t i = 0; i < cpu_counter; i++ )
                    {
                        if( (uint32_t)( g_acpi.srat_cpu_list[ i ].xapic_lapic ) == id )
                        {
                            // Found an existing entry for this processor, so just update the 8-bit identifier
                            g_acpi.srat_cpu_list[ i ].x2apic_lapic = id;
                            b_found = true;
                            break;
                        }
                    }
                }

                if( !b_found )
                {
                    // Create a new entry
                    struct axk_x86_srat_cpu_t* new_entry = g_acpi.srat_cpu_list + ( cpu_counter++ );

                    // Write info
                    new_entry->domain           = AXK_READ_UINT32( ptr_pos + 4UL );
                    new_entry->x2apic_lapic     = id;
                    new_entry->clock_domain     = AXK_READ_UINT32( ptr_pos + 16UL );
                }

                break;
            }

            default:
            break;
        }
    }

    return true;
}


bool parse_hpet( uint64_t address )
{
    struct axk_x86_acpi_header_t* ptr_header = (struct axk_x86_acpi_header_t*)( address );

    // Allocate hpet info structure and populate its fields
    g_acpi.hpet_info                        = AXK_MALLOC_TYPE( struct axk_x86_hpet_info_t );
    g_acpi.hpet_info->comparator_count      = ( AXK_READ_UINT8( address + 0x25UL ) & 0b11111 );
    g_acpi.hpet_info->is_large_counter      = ( ( AXK_READ_UINT8( address + 0x25UL ) & 0b100000 ) == 1 );
    g_acpi.hpet_info->is_legacy_replacment  = ( ( AXK_READ_UINT8( address + 0x25UL ) & 0b10000000 ) == 1 );
    g_acpi.hpet_info->pci_vendor            = AXK_READ_UINT16( address + 0x26UL );
    g_acpi.hpet_info->addr_space_type       = AXK_READ_UINT8( address + 0x28UL );
    g_acpi.hpet_info->reg_bit_width         = AXK_READ_UINT8( address + 0x29UL );
    g_acpi.hpet_info->reg_bit_offset        = AXK_READ_UINT8( address + 0x2AUL );
    g_acpi.hpet_info->address               = AXK_READ_UINT64( address + 0x2CUL );
    g_acpi.hpet_info->hpet_number           = AXK_READ_UINT8( address + 0x34UL );
    g_acpi.hpet_info->min_tick              = AXK_READ_UINT16( address + 0x35UL );
    g_acpi.hpet_info->page_protection       = AXK_READ_UINT8( address + 0x37UL );

    return true;
}


bool parse_rsdt( uint64_t address )
{
    // Perform validation and read the header
    if( address == 0UL ) { return false; }

    struct axk_x86_acpi_header_t* ptr_header = (struct axk_x86_acpi_header_t*)( address );
    if( !acpi_validate( address, "RSDT" ) ) { return false; }

    // Iterate through the entries, these point to other tables
    uint32_t table_count    = ( ptr_header->length - sizeof( struct axk_x86_acpi_header_t ) ) / 4U;
    bool b_found_madt       = false;
    bool b_found_fadt       = false;
    bool b_found_ssdt       = false;
    bool b_found_hpet       = false;

    for( uint32_t i = 0; i < table_count; i++ )
    {
        // Read the table address
        uint64_t table_address = (uint64_t)( *( (uint32_t*)( address + sizeof( struct axk_x86_acpi_header_t ) + ( i * 4u ) ) ) ) + AXK_KERNEL_VA_PHYSICAL;
        if( table_address == AXK_KERNEL_VA_PHYSICAL ) { break; }

        // Determine what table this is
        if( memcmp( (void*) table_address, "APIC", 4 ) == 0 )
        {
            b_found_madt = true;
            if( !parse_madt( table_address ) ) { return false; }
        }
        else if( memcmp( (void*) table_address, "FACP", 4 ) == 0 )
        {
            b_found_fadt = true;
            if( !parse_fadt( table_address ) ) { return false; }
        }
        else if( memcmp( (void*) table_address, "SSDT", 4 ) == 0 )
        {
            b_found_ssdt = true;
            if( !parse_ssdt( table_address ) ) { return false; }
        }
        else if( memcmp( (void*) table_address, "HPET", 4 ) == 0 )
        {
            if( b_found_hpet )
            {
                axk_basicout_prints( "ACPI: [Warning] Found multiple HPET tables\n" );
                continue;
            }

            b_found_hpet = true;
            if( !parse_hpet( table_address ) )
            {
                axk_basicout_prints( "ACPI: Failed to parse HPET table!\n" );
            }
        }
        else if( memcmp( (void*) table_address, "SRAT", 4 ) == 0 )
        {
            if( !parse_srat( table_address ) )
            {
                axk_basicout_prints( "ACPI: [Warning] Found SRAT, but was unable to parse!\n" );
                //return false;
            }
        }
    }

    // Ensure all the tables we need are present
    if( !b_found_madt || !b_found_fadt || !b_found_ssdt )
    {
        return false;
    }
    else
    {
        return true;
    }
}


bool parse_xsdt( uint64_t address )
{
    if( address == 0UL ) { return false; }

    struct axk_x86_acpi_header_t* ptr_header = (struct axk_x86_acpi_header_t*)( address );
    if( !acpi_validate( address, "XSDT" ) ) { return false; }

    bool b_found_madt = false;
    bool b_found_fadt = false;
    bool b_found_ssdt = false;
    bool b_found_hpet = false;

    uint32_t entry_count = ( ptr_header->length - sizeof( struct axk_x86_acpi_header_t ) ) / 8U;
    for( uint64_t i = 0UL; i < (uint64_t) entry_count; i++ )
    {
        // Read table address
        uint64_t table_address = *( (uint64_t*)( address + sizeof( struct axk_x86_acpi_header_t ) + ( i * 8UL ) ) );
        if( table_address == 0UL ) { break; }

        // Determine the table type..
        if( memcmp( (void*) table_address, "APIC", 4 ) == 0 )
        {
            b_found_madt = true;
            if( !parse_madt( table_address ) ) { return false; }
        }
        else if( memcmp( (void*) table_address, "FACP", 4 ) == 0 )
        {
            b_found_fadt = true;
            if( !parse_fadt( table_address ) ) { return false; }
        }
        else if( memcmp( (void*) table_address, "SSDT", 4 ) == 0 ) 
        {
            b_found_ssdt = true;
            if( !parse_ssdt( table_address ) ) { return false; }
        }
        else if( memcmp( (void*) table_address, "HPET", 4 ) == 0 )
        {
            if( b_found_hpet )
            {
                axk_basicout_prints( "ACPI: [Warning] Found multiple HPET tables\n" );
                continue;
            }

            b_found_hpet = true;
            if( !parse_hpet( table_address ) )
            {
                axk_basicout_prints( "ACPI: [Warning] Failed to parse HPET table!\n" );
            }
        }
    }

    // Ensure we found all the tables were lookin for
    if( !b_found_madt || !b_found_fadt || !b_found_ssdt )
    {
        return false;
    }
    else
    {
        return true;
    }
}


bool axk_x86_acpi_parse( void )
{
    // Guard against this being called twice
    if( g_init )
    {
        axk_panic( "ACPI: attempt to parse the tables twice" );
    }
    g_init = true;

    memset( (void*) &g_acpi, 0, sizeof( struct axk_x86_acpi_info_t ) );

    // Get info about ACPI from boot parameters
    const struct axk_bootparams_acpi_t* ptr_params = axk_bootparams_get_acpi();
    if( ptr_params->size == 0U )
    {
        return false;
    }

    // Validate the RSDP from the bootloader
    const char* valid_str = "RSD PTR ";
    if( memcmp( (void*) ptr_params->data, (void*) valid_str, 8 ) != 0 )
    {
        return false;
    }

    uint64_t accum = 0ul;
    for( uint64_t i = 0ul; i < sizeof( struct axk_x86_rsdp_v1_t ); i++ )
    {
        accum += ptr_params->data[ i ];
    }

    if( (uint8_t) accum != 0 ) { return false; }

    if( ptr_params->new_version )
    {
        accum = 0ul;
        for( uint64_t i = sizeof( struct axk_x86_rsdp_v1_t ); i < sizeof( struct axk_x86_rsdp_v2_t ); i++ )
        {
            accum += ptr_params->data[ i ];
        }

        if( (uint8_t)accum != 0 ) { return false; }
    }

    // Read version 1 field(s)
    struct axk_x86_rsdp_v1_t* ptr_rsdp1 = (struct axk_x86_rsdp_v1_t*)( ptr_params->data );
    g_acpi.system_mfgr[ 6 ] = '\0';
    memcpy( (void*) g_acpi.system_mfgr, (void*) ptr_rsdp1->oem_identifier, 6 );

    // Now, either parse XSDT or RSDT 
    if( ptr_params->new_version )
    {
        struct axk_x86_rsdp_v2_t* ptr_rsdp2 = (struct axk_x86_rsdp_v2_t*)( ptr_rsdp1 );
        if( !parse_xsdt( ptr_rsdp2->xsdt_address + AXK_KERNEL_VA_PHYSICAL ) )
        {
            return false;
        }
    }
    else
    {
        if( !parse_rsdt( (uint64_t)ptr_rsdp1->rsdt_address + AXK_KERNEL_VA_PHYSICAL ) )
        {
            return false;
        }
    }

    // Read info from CPUID about the processor
    if( !parse_cpuid() ) { return false; }

    // Print debug info
    axk_basicout_prints( "ACPI: Parsed " );
    axk_basicout_prints( ptr_params->new_version ? "XSDT" : "RSDT" );
    axk_basicout_prints( ". found " );
    axk_basicout_printu32( g_acpi.lapic_count );
    axk_basicout_prints( " LAPICS, " );
    axk_basicout_printu32( g_acpi.ioapic_count );
    axk_basicout_prints( " IOAPICS, " );
    axk_basicout_printu32( g_acpi.source_override_count );
    axk_basicout_prints( " external source overrides,\n\t " );
    axk_basicout_printu32( g_acpi.ioapic_nmi_count );
    axk_basicout_prints( " IOAPIC NMIs, " );
    axk_basicout_printu32( g_acpi.lapic_nmi_count );
    axk_basicout_prints( " LAPIC NMIs, HPET is " );
    if( g_acpi.hpet_info == NULL ) { axk_basicout_prints( "not " ); }
    axk_basicout_prints( "present\n\t SRAT Entries => Processors: " );
    axk_basicout_printu32( g_acpi.srat_cpu_count );
    axk_basicout_prints( ", Memory Ranges: " );
    axk_basicout_printu32( g_acpi.srat_memory_count );
    axk_basicout_printnl();
    
    return true;
}


struct axk_x86_acpi_info_t* axk_x86_acpi_get( void )
{
    return &g_acpi;
}


#endif
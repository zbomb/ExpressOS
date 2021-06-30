/*==============================================================
    Axon Kernel - Boot Parameters (x86)
    2021, Zachary Berry
    axon/arch_x86/boot/boot_params.c
==============================================================*/

#ifdef _X86_64_
#include "axon/boot/boot_params.h"
#include "axon/arch_x86/boot_params.h"
#include "axon/boot/multiboot2.h"
#include "axon/panic.h"
#include "axon/arch.h"
#include "axon/config.h"
#include <string.h>

/*
    Enums
*/
enum AXK_BOOTPARAMS_ERROR_X86_64
{
    BOOTPARAMS_ERROR_X86_64_SUCCESS                = 0,
    BOOTPARAMS_ERROR_X86_64_NODATA                 = 1,
    BOOTPARAMS_ERROR_X86_64_INVALIDHEADER          = 2,
    BOOTPARAMS_ERROR_X86_64_MISSINGINFO            = 3,
    BOOTPARAMS_ERROR_X86_64_INVALIDFRAMEBUFFER     = 4,
    BOOTPARAMS_ERROR_X86_64_INVALIDMEMORYMAP       = 5,
    BOOTPARAMS_ERROR_X86_64_INVALIDACPI            = 6,
    BOOTPARAMS_ERROR_X86_64_MISSINGFSDRIVER        = 7
};

/*
    ASM Functions
*/
extern uint64_t axk_get_kernel_offset( void );
extern uint64_t axk_get_kernel_size( void );

/*
    State
*/
static struct axk_bootparams_memorymap_t g_memorymap;
static struct axk_bootparams_framebuffer_t g_framebuffer;
static struct axk_bootparams_acpi_t g_acpi;


const struct axk_bootparams_memorymap_t* axk_bootparams_get_memorymap( void )
{
    return &g_memorymap;
}


const struct axk_bootparams_framebuffer_t* axk_bootparams_get_framebuffer( void )
{
    return &g_framebuffer;
}

const struct axk_bootparams_acpi_t* axk_bootparams_get_acpi( void )
{
    return &g_acpi;
}

bool parse_framebuffer( struct axk_multiboot_framebuffer_info_t* ptr_framebuffer, uint32_t size_framebuffer )
{
    // TODO: Support different starting framebuffers?
    return true;
}

bool parse_memorymap_helper( uint32_t* ptr_count, uint64_t begin, uint64_t end )
{
    // Loop through the sections, looking for issues
    for( uint32_t i = 0; i < *ptr_count; i++ )
    {
        struct axk_bootparams_memorymap_entry_t* ptr_entry = g_memorymap.entry_list + i;

        // Case 1: Section is 'inside' of the kernel.. probably shouldnt happen
        if( begin <= ptr_entry->begin && end >= ptr_entry->end )
        {
            // Were going to simply remove this section, and shift any following sections down to fix the array
            for( uint32_t k = i; k < ( *ptr_count - 1 ); k++ )
            {
                g_memorymap.entry_list[ k ] = g_memorymap.entry_list[ k + 1 ];
            }

            ( *ptr_count )--;
            i--;

        } // Case 2: Kernel overlaps the begining of the section, but not the end
        else if( begin <= ptr_entry->begin && end < ptr_entry->end && end > ptr_entry->begin )
        {
            // Clamp the begining of the section, to the end of the kernel
            ptr_entry->begin = end;

        } // Case 3: Kernel overlaps the end of the section, but not the begin
        else if( begin > ptr_entry->begin && end >= ptr_entry->end && begin < ptr_entry->end )
        {
            ptr_entry->end = begin;

        } // Case 4: Kernel is 'inside' of the section, but, with neither end or begin matching
        else if( begin > ptr_entry->begin && end < ptr_entry->end )
        {
            // This is more complicated... we need to split the memory section into two
            // The first thing we need to do, is add an additional section, but instead of adding it to the end
            // were just going to add it as the next entry, and shift the following ones up 
            if( *ptr_count + 1 > AXK_BOOT_MEMORYMAP_MAX_ENTRIES )
            {
                return false;
            }
            
            for( uint32_t k = *ptr_count; k > i + 1; k-- )
            {
                memcpy( (void*)( g_memorymap.entry_list + k ), (void*)( g_memorymap.entry_list + ( k - 1 ) ), sizeof( struct axk_bootparams_memorymap_entry_t ) );
            }

            ( *ptr_count )++;

            // Now, i + i can be written to
            struct axk_bootparams_memorymap_entry_t* ptr_high_entry = g_memorymap.entry_list + ( i + 1 );
            ptr_high_entry->end = ptr_entry->end;
            ptr_entry->end = begin;
            ptr_high_entry->begin = end;
            ptr_high_entry->status = ptr_entry->status;
            ptr_high_entry->flags = ptr_entry->flags;
        }
    }

    return true;
}

bool parse_memorymap( struct axk_multiboot_memorymap_t* ptr_memorymap, uint32_t size_memorymap )
{
    // Validate the parameters we were passed
    if( ptr_memorymap == NULL || size_memorymap == 0 || ptr_memorymap->entry_size == 0 )
    {
        return false;
    }

    // Determine the number of memory map sections
    uint32_t entry_size     = ptr_memorymap->entry_size;
    uint32_t entry_count    = ( size_memorymap - 8 ) / entry_size;

    if( entry_count > AXK_BOOT_MEMORYMAP_MAX_ENTRIES )
    {
        axk_panic( "Boot: There were too many sections in the system memory map (MAX: 128)" );
    }

    // Determine physical kernel start and end addresses
    uint64_t kernel_begin   = g_memorymap.kernel_offset - AXK_KERNEL_VA_IMAGE;
    uint64_t kernel_end     = kernel_begin + g_memorymap.kernel_size;
    uint64_t total_avail    = 0UL;

    // Read the memory map into our local structure
    uint32_t j = 0;
    for( uint32_t i = 0; i < entry_count; i++ )
    {
        struct axk_multiboot_memorymap_entry_t* ptr_entry = (struct axk_multiboot_memorymap_entry_t*)( ( (uint8_t*) ptr_memorymap ) + 8 + ( i * entry_size ) );

        // Validate this entry
        if( ptr_entry->length == 0 || ptr_entry->_rsvd_1_ != 0 )
        {
            return false;
        }

        // Copy into the local array
        struct axk_bootparams_memorymap_entry_t* ptr_local_entry = g_memorymap.entry_list + j++;
        ptr_local_entry->begin = ptr_entry->base_addr;
        ptr_local_entry->end = ptr_entry->base_addr + ptr_entry->length;

        switch( ptr_entry->type )
        {
            case MULTIBOOT_MEM_TYPE_AVAIL_RAM:
            ptr_local_entry->status = MEMORYMAP_ENTRY_STATUS_AVAILABLE;
            total_avail += ptr_entry->length;
            break;
            case MULTIBOOT_MEM_TYPE_AVAIL_ACPI:
            ptr_local_entry->status = MEMORYMAP_ENTRY_STATUS_ACPI;
            total_avail += ptr_entry->length;
            break;
            default:
            ptr_local_entry->status = MEMORYMAP_ENTRY_STATUS_RESERVED;
            break;
        }
    }

    g_memorymap.total_mem = total_avail;

    // Ensure theres enough RAM to meet system requirments
    if( total_avail < AXK_MINREQ_MEMORY )
    {
        axk_panic( "Boot: the system doesnt meet minimum memory requirments!" );
    }

    // Now, were going to clean up any sections that are above physical memory
    bool b_found_avail = false;
    for( uint32_t i = j; i > 0; i-- )
    {
        struct axk_bootparams_memorymap_entry_t* ptr_local_entry = g_memorymap.entry_list + ( i - 1 );
        // Check if this section is available
        if( ptr_local_entry->status == MEMORYMAP_ENTRY_STATUS_AVAILABLE || 
            ptr_local_entry->status == MEMORYMAP_ENTRY_STATUS_ACPI )
        {
            b_found_avail = true;
        }
        else if( !b_found_avail )
        {
            // Remove this section
            ptr_local_entry->begin      = 0UL;
            ptr_local_entry->end        = 0UL;
            ptr_local_entry->flags      = 0;
            ptr_local_entry->status     = MEMORYMAP_ENTRY_STATUS_RESERVED;

            j--;
        }
    }

    // Next up, we need to ensure the kernel and initrd arent overlapping available sections
    if( !parse_memorymap_helper( &j, kernel_begin, kernel_end ) ||
        !parse_memorymap_helper( &j, g_memorymap.initrd_offset, g_memorymap.initrd_offset + g_memorymap.initrd_size ) )
    {
        axk_panic( "Boot: There were too many sections in the system memory map (MAX: 128)" );
    }

    // Add the kernel section
    g_memorymap.entry_list[ j ].begin       = kernel_begin;
    g_memorymap.entry_list[ j ].end         = kernel_end;
    g_memorymap.entry_list[ j++ ].status    = MEMORYMAP_ENTRY_STATUS_KERNEL;

    // Add the ramdisk section
    g_memorymap.entry_list[ j ].begin       = g_memorymap.initrd_offset;
    g_memorymap.entry_list[ j ].end         = g_memorymap.initrd_offset + g_memorymap.initrd_size;
    g_memorymap.entry_list[ j++ ].status    = MEMORYMAP_ENTRY_STATUS_RAMDISK;

    g_memorymap.entry_count = j;

    // Finally, we need to sort the memory map entries in order of start address
    for( uint32_t i = 0; i < j - 1; i++ )
    {
        // Find the minimum entry index from [i,j)
        uint32_t min_index = i;
        for( uint32_t k = i; k < j; k++ )
        {
            if( g_memorymap.entry_list[ k ].begin < g_memorymap.entry_list[ min_index ].begin )
            {
                min_index = k;
            }
        }

        if( min_index != i )
        {
            // Swap the entries
            struct axk_bootparams_memorymap_entry_t temp = g_memorymap.entry_list[ i ];
            memcpy( (void*)( g_memorymap.entry_list + i ), (void*)( g_memorymap.entry_list + min_index ), sizeof( struct axk_bootparams_memorymap_entry_t ) );
            g_memorymap.entry_list[ min_index ] = temp;
        }
    }

    // DEBUG, Print some info
    /*
    axk_terminal_prints( "=====> Physical Memory Sections <=======\n" );
    for( uint32_t i = 0; i < j; i++ )
    {
        if( i > 8 ) { break; }
        //if( i < 8 ) { continue; }
        axk_terminal_printtab();
        axk_terminal_printu32( i );
        axk_terminal_prints( ":\t" );

        struct axk_bootparams_memorymap_entry_t* ptr_section = g_memorymap.entry_list + i;
        switch( ptr_section->status )
        {
            case MEMORYMAP_ENTRY_STATUS_AVAILABLE:
            axk_terminal_prints( "AVAILABLE\n" );
            break;

            case MEMORYMAP_ENTRY_STATUS_KERNEL:
            axk_terminal_prints( "KERNEL\n" );
            break;

            case MEMORYMAP_ENTRY_STATUS_RESERVED:
            axk_terminal_prints( "RESERVED\n" );
            break;

            case MEMORYMAP_ENTRY_STATUS_ACPI:
            axk_terminal_prints( "ACPI\n" );
            break;

            case MEMORYMAP_ENTRY_STATUS_RAMDISK:
            axk_terminal_prints( "RAM DISK\n" );
            break;
        }

        axk_terminal_printtab();
        axk_terminal_printtab();

        axk_terminal_prints( "Begin: " );
        axk_terminal_printh64_lz( ptr_section->begin, true );
        axk_terminal_prints( "\tEnd: " );
        axk_terminal_printh64_lz( ptr_section->end, true );
        axk_terminal_printnl();
    }
    */
    // END DEBUG

    return true;
}

enum AXK_BOOTPARAMS_ERROR_X86_64 do_parse( void* ptr_multiboot )
{
    // Clear the globals
    memset( (void*) &g_memorymap, 0, sizeof( g_memorymap ) );
    memset( (void*) &g_acpi, 0, sizeof( g_acpi ) );
    memset( (void*) &g_framebuffer, 0, sizeof( g_framebuffer ) );

    // First, we need to ensure the pointer is valid
    if( ptr_multiboot == NULL ) { return BOOTPARAMS_ERROR_X86_64_NODATA; } 

    // Check for a valid header
    struct axk_multiboot_header_t* ptr_header = (struct axk_multiboot_header_t*) ptr_multiboot;
    if( ptr_header->total_size > 0xF000 || ptr_header->total_size < 0x10 || ptr_header->_rsvd_1_ != 0x00 )
    {
        return BOOTPARAMS_ERROR_X86_64_INVALIDHEADER;
    }

    uint8_t* ptr_params_end     = ( (uint8_t*) ptr_multiboot ) + ptr_header->total_size;
    uint8_t* ptr_position       = ( (uint8_t*) ptr_multiboot ) + sizeof( struct axk_multiboot_header_t );

    // Store some info about where things are loaded in physical memory
    g_memorymap.kernel_offset  = axk_get_kernel_offset();
    g_memorymap.kernel_size    = axk_get_kernel_size();

    // There are a few structures we need to look out for in the list
    struct axk_multiboot_memorymap_t* ptr_memmap = NULL;
    uint32_t size_memmap = 0;

    struct axk_multiboot_framebuffer_info_t* ptr_framebuffer = NULL;
    uint32_t size_framebuffer = 0;

    struct axk_multiboot_modules_t* ptr_module = NULL;

    // Start iterating through the tags
    while( ptr_position < ptr_params_end )
    {
        // Read the next tag
        struct axk_multiboot_tag_t* ptr_tag = (struct axk_multiboot_tag_t*) ptr_position;
        ptr_position += sizeof( struct axk_multiboot_tag_t );

        switch( ptr_tag->type )
        {
            case MULTIBOOT_TAG_END:
            {
                ptr_position = ptr_params_end;
                break;
            }
            case MULTIBOOT_TAG_MEMORY_MAP:
            {
                ptr_memmap = (struct axk_multiboot_memorymap_t*) ptr_position;
                size_memmap = ptr_tag->size;
                break;
            }
            case MULTIBOOT_TAG_FRAMEBUFFER_INFO:
            {
                ptr_framebuffer = (struct axk_multiboot_framebuffer_info_t*) ptr_position;
                size_framebuffer = ptr_tag->size;
                break;
            }
            case MULTIBOOT_TAG_IMAGE_LOAD_BASE_PHYS_ADDRESS:
            {
                // TODO: Do we need this?
                break;
            }
            case MULTIBOOT_TAG_ACPI_NEW_RSDP:
            {
                // Copy info into a local structure
                if( ptr_tag->size > 48 )
                {
                    return BOOTPARAMS_ERROR_X86_64_INVALIDACPI;
                }

                memcpy( (void*) g_acpi.data, (void*) ptr_position, ptr_tag->size );
                g_acpi.new_version = true;
                g_acpi.size = ptr_tag->size;
                break;
            }
            case MULTIBOOT_TAG_ACPI_OLD_RSDP:
            {
                // Copy info into local structure
                if( ptr_tag->size > 48 )
                {
                    return BOOTPARAMS_ERROR_X86_64_INVALIDACPI;
                }

                memcpy( (void*) g_acpi.data, (void*) ptr_position, ptr_tag->size );
                g_acpi.new_version = false;
                g_acpi.size = ptr_tag->size;
                break;
            }
            case MULTIBOOT_TAG_MODULES:
            {
                // TODO: Ensure this is the initrd module
                ptr_module = (struct axk_multiboot_modules_t*) ptr_position;
                break;
            }
        }

        // Advance to the next tag, taking alignment into account
        ptr_position += ( ptr_tag->size - sizeof( struct axk_multiboot_tag_t) );
        if( ( (uint64_t) ptr_position ) % 8 != 0 )
        {
            ptr_position += ( 8 - ( ( (uint64_t) ptr_position ) % 8 ) );
        }
    }

    // Check to see if we found everything we need
    if( ptr_memmap == NULL || ptr_framebuffer == NULL || size_memmap == 0 || size_framebuffer == 0 )
    {
        return BOOTPARAMS_ERROR_X86_64_MISSINGINFO;
    }

    // Check to see if filesystem driver ramdisk is present
    if( ptr_module == NULL || ( ptr_module->mod_end - ptr_module->mod_start ) == 0UL )
    {
        return BOOTPARAMS_ERROR_X86_64_MISSINGFSDRIVER;
    }

    g_memorymap.initrd_offset = (uint64_t) ptr_module->mod_start;
    g_memorymap.initrd_size = (uint64_t)( ptr_module->mod_end - ptr_module->mod_start );

    // Parse the framebuffer info
    if( !parse_framebuffer( ptr_framebuffer, size_framebuffer ) )
    {
        return BOOTPARAMS_ERROR_X86_64_INVALIDFRAMEBUFFER;
    }

    // Parse the memory map
    if( !parse_memorymap( ptr_memmap, size_memmap ) )
    {
        return BOOTPARAMS_ERROR_X86_64_INVALIDMEMORYMAP;
    }

    // Ensure ACPI was found
    if( g_acpi.size == 0 )
    {
        return BOOTPARAMS_ERROR_X86_64_INVALIDACPI;
    }

    return BOOTPARAMS_ERROR_X86_64_SUCCESS;
}

void ax_bootparams_parse( void* ptr_info )
{
    axk_terminal_clear(); // DEBUG
    enum AXK_BOOTPARAMS_ERROR_X86_64 err = do_parse( ptr_info );

    switch( err )
    {
        case BOOTPARAMS_ERROR_X86_64_NODATA:
        axk_panic( "Boot: Failed to parse boot parameters, no/invalid data" );
        break;
        case BOOTPARAMS_ERROR_X86_64_MISSINGINFO:
        axk_panic( "Boot: Failed to parse boot parameters, missing info" );
        break;
        case BOOTPARAMS_ERROR_X86_64_MISSINGFSDRIVER:
        axk_panic( "Boot: Failed to parse boot parameters, missing filesystem driver" );
        break;
        case BOOTPARAMS_ERROR_X86_64_INVALIDMEMORYMAP:
        axk_panic( "Boot: Failed to parse boot parameters, invalid memory map" );
        break;
        case BOOTPARAMS_ERROR_X86_64_INVALIDHEADER:
        axk_panic( "Boot: Failed to parse boot parameters, invalid header" );
        break;
        case BOOTPARAMS_ERROR_X86_64_INVALIDFRAMEBUFFER:
        axk_panic( "Boot: Failed to parse boot parameters, invalid frame buffer" );
        break;
        case BOOTPARAMS_ERROR_X86_64_INVALIDACPI:
        axk_panic( "Boot: Failed to parse boot parameters, invalid ACPI table" );
        break;
    }
}

#endif
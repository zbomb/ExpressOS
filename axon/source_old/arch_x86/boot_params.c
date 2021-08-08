/*==============================================================
    Axon Kernel - Boot Parameters (x86)
    2021, Zachary Berry
    axon/arch_x86/boot/boot_params.c
==============================================================*/

#ifdef _X86_64_
#include "axon/boot/boot_params.h"
#include "axon/arch_x86/boot_params.h"
#include "axon/arch_x86/acpi_info.h"

/*
    Constants
*/
#define EFI_MEMORY_UC               0x0000000000000001  // Supports being configured as 'non-cachable'
#define EFI_MEMORY_WC               0x0000000000000002  // Supports being configured as 'write-combining' caching
#define EFI_MEMORY_WT               0x0000000000000004  // Supports being configured as 'write-through' caching
#define EFI_MEMORY_WB               0x0000000000000008  // Supports being configured as 'write-back' caching
#define EFI_MEMORY_UCE              0x0000000000000010  // Supports being configured as 'non-cachable', 'exported' and supports 'fetch_add' semaphore mechanism
#define EFI_MEMORY_WP               0x0000000000001000  // Supports being configured as 'write-protected'
#define EFI_MEMORY_RP               0x0000000000002000  // Supports being configured as 'read-protected'
#define EFI_MEMORY_XP               0x0000000000004000  // Supports being configured as 'non-executable'
#define EFI_MEMORY_NV               0x0000000000008000  // This memory range is persistent
#define EFI_MEMORY_MORE_RELIABLE    0x0000000000010000  // This memory range has higher reliability than the other memory installed
#define EFI_MEMORY_RO               0x0000000000020000  // Supports being configured as 'read-only'
#define EFI_MEMORY_RUNTIME          0x8000000000000000  // This memory region needs to be given a virtual mapping by the OS when 'SetVirtualAddressMap' is called

/*
    ASM Functions
*/
extern uint64_t axk_x86_get_kernel_begin( void );
extern uint64_t axk_x86_get_kernel_end( void );

/*
    State
*/
static struct axk_bootparams_memorymap_t g_memorymap;
static struct axk_bootparams_framebuffer_t g_framebuffer;
static struct axk_bootparams_acpi_t g_acpi;

/*
    Functions
*/

bool parse_framebuffer( struct tzero_payload_params_t* pparams );
bool parse_memorymap( struct tzero_payload_params_t* pparams );
bool parse_memorymap_helper( uint32_t* ptr_count, uint64_t begin, uint64_t end );


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


bool axk_x86_bootparams_parse( struct tzero_payload_params_t* ptr_params, struct axk_error_t* out_err )
{
    // Check for null, shouldnt happen though because we validated in ASM
    if( ptr_params == NULL || ptr_params->framebuffer_ptr == NULL || ptr_params->rsdp_ptr == NULL || 
        ptr_params->fn_load_success == NULL || ptr_params->fn_load_failed == NULL )
    {
        axk_error_reset( out_err, AXK_ERROR_UNKNOWN );
        axk_error_write_str( out_err, "The parameters were missing requires elements" );
        return false;
    }

    // Clear the globals, ensure they are zeroed
    memset( &g_memorymap, 0, sizeof( g_memorymap ) );
    memset( &g_framebuffer, 0, sizeof( g_framebuffer ) );
    memset( &g_acpi, 0, sizeof( g_acpi ) );

    // Set some static values
    g_memorymap.initrd_offset   = 0x00UL;
    g_memorymap.initrd_size     = 0x00UL;
    g_memorymap.kernel_offset   = axk_x86_get_kernel_begin();
    g_memorymap.kernel_size     = axk_x86_get_kernel_end() - g_memorymap.kernel_offset;

    // Parse framebuffer info to arch-indepdendent structure
    struct tzero_payload_params_t* pparams = (struct tzero_payload_params_t*)( ptr_params );
    if( !parse_framebuffer( pparams ) )
    {
        axk_error_reset( out_err, AXK_ERROR_UNKNOWN );
        axk_error_write_str( out_err, "The framebuffer couldnt be parsed!" );
        return false;
    }
    
    // Parse the memory map into an arch-idependent structure
    if( !parse_memorymap( pparams ) )
    {
        axk_error_reset( out_err, AXK_ERROR_UNKNOWN );
        axk_error_write_str( out_err, "The memory-map couldnt be parsed!" );
        return false;
    }

    // Store ACPI info
    g_acpi.size         = pparams->b_new_rsdp ? sizeof( struct axk_x86_rsdp_v2_t ) : sizeof( struct axk_x86_rsdp_v1_t );
    g_acpi.new_version  = pparams->b_new_rsdp;

    memcpy( (void*) g_acpi.data, pparams->rsdp_ptr, g_acpi.size );

    return true;
}


bool parse_framebuffer( struct tzero_payload_params_t* pparams )
{
    // First, read in the list of available resolutions (or at least the top 128 resolutions)
    g_framebuffer.resolution_count = pparams->res_count;

    for( uint32_t i = ( g_framebuffer.resolution_count > 128 ? 128 : g_framebuffer.resolution_count ) - 1; i >= 0; i-- )
    {
        g_framebuffer.resolution_list[ i ].id                   = pparams->res_list[ i ].index;
        g_framebuffer.resolution_list[ i ].width                = pparams->res_list[ i ].width;
        g_framebuffer.resolution_list[ i ].height               = pparams->res_list[ i ].height;
        g_framebuffer.resolution_list[ i ].pixels_per_scanline  = pparams->res_list[ i ].pixels_per_scanline;
        
        switch( pparams->res_list[ i ].format )
        {
            case efi_pixel_rgbx_32:
            g_framebuffer.resolution_list[ i ].format = PIXEL_FORMAT_RGBX_32;
            break;

            case efi_pixel_bgrx_32:
            g_framebuffer.resolution_list[ i ].format = PIXEL_FORMAT_BGRX_32;
            break;
        }

        if( i == 0 ) { break; }
    }

    // Next, we need to move in other framebuffer data
    g_framebuffer.buffer                            = pparams->framebuffer_ptr;
    g_framebuffer.size                              = pparams->framebuffer_size;
    g_framebuffer.resolution.width                  = pparams->framebuffer_res.width;
    g_framebuffer.resolution.height                 = pparams->framebuffer_res.height;
    g_framebuffer.resolution.pixels_per_scanline    = pparams->framebuffer_res.pixels_per_scanline;
    g_framebuffer.resolution.id                     = pparams->framebuffer_res.index;
    
    switch( pparams->framebuffer_res.format )
    {
        case efi_pixel_rgbx_32:
        g_framebuffer.resolution.format = PIXEL_FORMAT_RGBX_32;
        break;

        case efi_pixel_bgrx_32:
        g_framebuffer.resolution.format = PIXEL_FORMAT_BGRX_32;
        break;
    }

    // Clear any outstanding data in the buffer
    uint8_t* pbuffer = (uint8_t*)( g_framebuffer.buffer );
    for( size_t i = 0; i < g_framebuffer.size; i++ )
    {
        pbuffer[ i ] = 0x00;
    }

    return true;
}


bool parse_memorymap( struct tzero_payload_params_t* pparams )
{
    // The UEFI memory map is a list of entries, each giving us info on a range of pages
    // We will then convert these into a structure of our own, and take into account known structures in memory, such as the kernel and ramdisk
    uint8_t* map_iter       = (uint8_t*)( pparams->memmap_ptr );
    uint8_t* map_end        = map_iter + pparams->memmap_size;
    uint64_t avail_pages    = 0UL;

    // TODO: Find a way to output an error message
    g_memorymap.entry_count = 0;

    while( map_iter < map_end )
    {
        // Determine the next entry type, and if we can combine with the last entry to avoid extra entries
        struct tzero_memory_map_entry_t* pentry = (struct tzero_memory_map_entry_t*)( map_iter );
        enum AXK_MEMORYMAP_ENTRY_STATUS status;

        switch( pentry->type )
        {
            // These ranges might have been used by T-0, but should be usable after we parse this map
            case EfiLoaderCode:
            /* fall through */
            case EfiLoaderData:
            status = MEMORYMAP_ENTRY_STATUS_AVAILABLE;
            avail_pages += pentry->page_count;
            break;

            // These ranges are good to use
            case EfiBootServicesCode:
            /* fall through */
            case EfiBootServicesData: 
            /* fall through */
            case EfiConventionalMemory:
            /* fall through */
            case EfiPersistentMemory:
            status = MEMORYMAP_ENTRY_STATUS_AVAILABLE;
            avail_pages += pentry->page_count;
            break;

            // ACPI reclaimable memory
            case EfiACPIReclaimMemory:
            status = MEMORYMAP_ENTRY_STATUS_ACPI;
            avail_pages += pentry->page_count;
            break;

            // This is non-usable memory
            case EfiReservedMemoryType:
            /* fall through */
            case EfiRuntimeServicesCode:
            /* fall through */
            case EfiRuntimeServicesData:
            /* fall through */
            case EfiUnusableMemory:
            /* fall through */
            case EfiACPIMemoryNVS:
            /* fall through */
            case EfiMemoryMappedIO:
            /* fall through */
            case EfiMemoryMappedIOPortSpace:
            /* fall through */
            case EfiPalCode:
            /* fall through */
            default:
            status = MEMORYMAP_ENTRY_STATUS_RESERVED;
            break;
        }

        uint64_t begin  = pentry->physical_address;
        uint64_t end    = begin + ( pentry->page_count * 0x1000UL );

        // Search if any previous entries are adjacent to this entry so we can combine them
        bool b_expanded = false;
        for( uint32_t i = 0; i < g_memorymap.entry_count; i++ )
        {
            struct axk_bootparams_memorymap_entry_t* pp = g_memorymap.entry_list + i;
            if( pp->status == status )
            {
                if( pp->begin == end )
                {
                    pp->begin = begin;
                    b_expanded = true;
                    break;
                }
                else if( pp->end == begin )
                {
                    pp->end = end;
                    b_expanded = true;
                    break;
                }
            }
        }

        // If we didnt expand an existing entry, then we need to create a new one
        if( !b_expanded )
        {
            struct axk_bootparams_memorymap_entry_t* pp = g_memorymap.entry_list + ( g_memorymap.entry_count++ );

            pp->begin   = begin;
            pp->end     = end;
            pp->status  = status;
        }

        map_iter += pparams->memmap_desc_size;

        // Check if we hit the max entry count
        if( g_memorymap.entry_count >= AXK_BOOT_MEMORYMAP_MAX_ENTRIES )
        {
            // TODO: How to display better error messages?
            return false;
        }
    }

    // Now, we want to ensure any memory used by the kernel or ramdisk are marked as such
    if( !parse_memorymap_helper( &g_memorymap.entry_count, g_memorymap.kernel_offset, g_memorymap.kernel_offset + g_memorymap.kernel_size ) ||
        !parse_memorymap_helper( &g_memorymap.entry_count, g_memorymap.initrd_offset, g_memorymap.initrd_offset + g_memorymap.initrd_size ) )
    {
        // TODO: How to display error messages?
        return false;
    }

    // Add the kernel section
    g_memorymap.entry_list[ g_memorymap.entry_count ].begin     = g_memorymap.kernel_offset;
    g_memorymap.entry_list[ g_memorymap.entry_count ].end       = g_memorymap.kernel_offset + g_memorymap.kernel_size;
    g_memorymap.entry_list[ g_memorymap.entry_count++ ].status  = MEMORYMAP_ENTRY_STATUS_KERNEL;

    // Add the ramdisk section
    g_memorymap.entry_list[ g_memorymap.entry_count ].begin     = g_memorymap.initrd_offset;
    g_memorymap.entry_list[ g_memorymap.entry_count ].end       = g_memorymap.initrd_offset + g_memorymap.initrd_size;
    g_memorymap.entry_list[ g_memorymap.entry_count++ ].status  = MEMORYMAP_ENTRY_STATUS_RAMDISK;

    // Finally, we need to sort the memory map entries in order of start address
    for( uint32_t i = 0; i < g_memorymap.entry_count - 1; i++ )
    {
        // Find the minimum entry index from [i, g_memorymap.entry_count)
        uint32_t min_index = i;
        for( uint32_t k = i; k < g_memorymap.entry_count; k++ )
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

#endif
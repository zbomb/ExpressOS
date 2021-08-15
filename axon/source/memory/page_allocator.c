/*==============================================================
    Axon Kernel - Physical Page Allocator
    2021, Zachary Berry
    axon/source/memory/page_allocator.c
==============================================================*/

#include "axon/memory/memory_private.h"
#include "axon/memory/page_allocator.h"
#include "axon/kernel/panic.h"
#include "axon/gfx/basic_terminal.h"
#include "axon/library/spinlock.h"


/*
    Page Info Structure
*/
struct axk_page_info_t
{
    uint32_t process_id;
    uint8_t state;
    uint8_t type;
};

/*
    State
*/
static bool g_init              = false;
static uint8_t* g_page_list     = NULL;
static uint64_t g_page_count    = 0UL;

static struct axk_spinlock_t g_lock;


/*
    Function Implementations
*/
void axk_page_allocator_init( struct tzero_payload_parameters_t* in_params )
{
    // Ensure we have the information required to initialize
    if( in_params == NULL || in_params->memory_map.list == NULL || in_params->memory_map.count == 0UL )
    {
        axk_panic( "Failed to initialize physical page allocator. Required information from bootloader was not present, check for corrupt installation" );
    }

    // Guard against this being called twice
    if( g_init ) { return; }
    g_init = true;

    axk_spinlock_init( &g_lock );

    // Determine the total number of pages we need to track state for
    // We do this by finding the highest non-reserved page
    uint64_t highest_available_page     = 0UL;
    uint64_t highest_total_page         = 0UL;

    for( uint32_t i = 0; i < in_params->memory_map.count; i++ )
    {
        struct tzero_memory_entry_t* entry = in_params->memory_map.list + i;
        if( entry->type == TZERO_MEMORY_AVAILABLE || entry->type == TZERO_MEMORY_ACPI || entry->type == TZERO_MEMORY_BOOTLOADER )
        {
            uint64_t highest_page = ( entry->base_address / AXK_PAGE_SIZE ) + ( entry->page_count );
            highest_available_page = highest_available_page < highest_page ? highest_page : highest_available_page;
        }

        if( i == in_params->memory_map.count - 1UL )
        {
            highest_total_page = ( entry->base_address / AXK_PAGE_SIZE ) + ( entry->page_count );
        }
    }

    // The number of bytes needed for the structure, is 6 bytes per page. We need to scan the memory map to find a place to write this to
    uint64_t page_info_size     = highest_available_page * 6UL;
    uint64_t page_info_pages    = ( page_info_size + AXK_PAGE_SIZE - 1UL ) / AXK_PAGE_SIZE;
    uint64_t page_info_addr     = 0UL;

    for( uint32_t i = 0; i < in_params->memory_map.count; i++ )
    {
        struct tzero_memory_entry_t* entry = in_params->memory_map.list + i;
        if( entry->type == TZERO_MEMORY_AVAILABLE )
        {
            uint64_t entry_end  = entry->base_address + ( entry->page_count * AXK_PAGE_SIZE );

            // We need to ensure we dont mess with 0x8000 to 0x9000, since we are using this space for processor initialization
            // To do this, we set the minimum base address as 0x9000
            if( entry->base_address < 0x8000 )
            {
                if( 0x9000 + page_info_size <= entry_end )
                {
                    page_info_addr = 0x9000;
                    break;
                }
            }
            else
            {
                if( entry->base_address + page_info_size <= entry_end )
                {
                    page_info_addr = entry->base_address;
                    break;
                }
            }
        }
    }

    // Check if we couldnt find anywhere to place this structure
    if( page_info_addr == 0UL )
    {
        axk_panic_begin();
        axk_panic_prints( "Failed to initialize physical page allocator. Couldnt find a continuous block of memory to write memory tracking structure (" );
        axk_panic_printn( page_info_size / 1024 );
        axk_panic_prints( "KB)" );
        axk_panic_end();
    }
    else
    {
        g_page_list     = (uint8_t*)( page_info_addr );
        g_page_count    = highest_available_page;
    }

    // Loop through each physical page in the system, and determine its state
    uint64_t kernel_begin       = axk_get_kernel_offset() - AXK_KERNEL_VA_IMAGE;
    uint64_t kernel_end         = kernel_begin + axk_get_kernel_size();

    uint64_t kernel_page_count      = 0UL;
    uint64_t avail_page_count       = 0UL;

    for( uint64_t i = 0; i < highest_available_page; i++ )
    {
        uint64_t page_begin                 = ( i * AXK_PAGE_SIZE );
        uint64_t page_end                   = page_begin + AXK_PAGE_SIZE;
        uint8_t type                        = AXK_PAGE_TYPE_OTHER;
        uint8_t state                       = AXK_PAGE_STATE_RESERVED;
        uint32_t process_id                 = AXK_PROCESS_INVALID;

        // Check if this page is part of the kernel image
        if( ( page_begin >= kernel_begin && page_begin < kernel_end ) ||
            ( page_end >= kernel_begin && page_end <= kernel_end ) )
        {
            type        = AXK_PAGE_TYPE_IMAGE;
            state       = AXK_PAGE_STATE_RESERVED;
            process_id  = AXK_PROCESS_KERNEL;

            kernel_page_count++;
        }
        else
        {
            // Scan the memory map to determine the page state
            for( uint32_t i = 0; i < in_params->memory_map.count; i++ )
            {
                struct tzero_memory_entry_t* entry = in_params->memory_map.list + i;
                uint64_t entry_end = entry->base_address + ( entry->page_count * AXK_PAGE_SIZE );
                
                if( page_begin >= entry->base_address && page_begin < entry_end )
                {
                    switch( entry->type )
                    {
                        case TZERO_MEMORY_ACPI:
                        state = AXK_PAGE_STATE_ACPI;
                        avail_page_count++;
                        break;

                        case TZERO_MEMORY_AVAILABLE:
                        state = AXK_PAGE_STATE_AVAILABLE;
                        avail_page_count++;
                        break;

                        case TZERO_MEMORY_BOOTLOADER:
                        state = AXK_PAGE_STATE_BOOTLOADER;
                        avail_page_count++;
                        break;

                        case TZERO_MEMORY_MAPPED_IO:
                        state = AXK_PAGE_STATE_RESERVED;
                        break;

                        case TZERO_MEMORY_RESERVED:
                        state = AXK_PAGE_STATE_RESERVED;
                        break;
                    }

                    break;
                }
            }
        }

        // Write the process identifier, then state, then type
        struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( i * 6UL ) );
        
        page_info->process_id   = process_id;
        page_info->state        = state;
        page_info->type         = type;
    }

    // There are some pages that have a 'static state'
    // i.e. Page 0x08 is reserved for AP init
    struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( 8UL * 6UL ) );

    page_info->state        = AXK_PAGE_STATE_RESERVED;
    page_info->type         = AXK_PAGE_TYPE_OTHER;
    page_info->process_id   = AXK_PROCESS_INVALID;

    axk_basicterminal_prints( "Page Allocator: Initialized successfully. Total Pages: " );
    axk_basicterminal_printu64( highest_available_page );
    axk_basicterminal_prints( ",  Kernel Size: " );
    axk_basicterminal_printu64( ( kernel_page_count * AXK_PAGE_SIZE ) / 1024UL );
    axk_basicterminal_prints( "KB  Available Memory: " );
    axk_basicterminal_printu64( ( ( avail_page_count * AXK_PAGE_SIZE ) / 1024UL ) / 1024UL );
    axk_basicterminal_prints( "MB\n" );
}


bool axk_page_acquire( uint64_t count, uint64_t* out_page_list, uint32_t process_id, uint8_t type, uint32_t flags )
{
    // IMPROVE: Once we fail to find consecutive pages for the allocation requetst, we fall back on just finding any available pages
    // Instead, we should loop and allocate the largest available consecutive group of pages until we hit the target amount, until there are no pages left
    // IMPROVE: We also should allow the caller to specify they want to find physical pages near/adjacent with a target page

    // Validate the parameters
    if( count == 0UL || out_page_list == NULL || process_id == AXK_PROCESS_INVALID ) { return false; }

    // Check for relevant flags
    bool b_clear        = AXK_CHECK_FLAG( flags, AXK_PAGE_FLAG_CLEAR );
    bool b_prefer_high  = AXK_CHECK_FLAG( flags, AXK_PAGE_FLAG_PREFER_HIGH );
    bool b_consecutive  = AXK_CHECK_FLAG( flags, AXK_PAGE_FLAG_CONSECUTIVE );

    // Start searching for available pages we can acquire
    uint8_t cons_search_state       = 0;
    uint64_t range_start            = 0UL;
    uint64_t range_count            = 0UL;
    uint64_t largest_range_index    = 0UL;
    uint64_t largest_range_count    = 0UL;

    // Acquire lock on the page allocator state
    axk_spinlock_acquire( &g_lock );

    for( uint64_t i = 0; i < g_page_count; i++ )
    {
        uint64_t index = b_prefer_high ? g_page_count - i - 1UL : i;
        struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( index * 6UL ) );

        // We want to attempt to find a consecutive page range, and not have severe fragmentation
        if( cons_search_state == 0 && page_info->state == AXK_PAGE_STATE_AVAILABLE )
        {
            // State 0: Searching for the first available page
            range_start         = index;
            range_count         = 1UL;
            cons_search_state   = 1;

            // Check if were only looking for a single page
            if( count == 1UL )
            {
                break;
            }
        }
        else if( cons_search_state == 1 )
        {
            // State 1: Ensuring the pages after the 'range start' are all available
            if( page_info->state == AXK_PAGE_STATE_AVAILABLE )
            {
                range_count++;

                // Check if we have enough pages
                if( range_count >= count )
                {
                    break;
                }
                else
                {
                    // Check if the range is the largest weve found so far
                    if( range_count > largest_range_count )
                    {
                        largest_range_count = range_count;
                        largest_range_index = b_prefer_high ? index : index - range_count + 1UL;
                    }
                }
            }
            else
            {
                // Go back to looking for the first available page in the range (state 0)
                range_start         = 0UL;
                range_count         = 0UL;
                cons_search_state   = 0;
            }
        }
    }

    // Check if we didnt find enough consecutive pages
    if( range_count < count )
    {
        if( b_consecutive ) { return false; }

        // So, lets save the indicies of the pages of the 'largest found range'
        uint64_t range_begin        = largest_range_index;
        uint64_t range_end          = range_begin + largest_range_count;
        uint64_t ii                 = 0UL;

        for( uint64_t i = range_begin; i < range_end; i++ )
        {
            out_page_list[ ii++ ] = i;
        }

        // And now, were going to seek the remaining required pages
        for( uint64_t i = 0; i < g_page_count; i++ )
        {
            uint64_t index = b_prefer_high ? g_page_count - i - 1UL : i;
            struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( index * 6UL ) );

            // Skip pages from the large range we already selected
            if( i >= range_begin && i < range_end ) { continue; }

            if( page_info->state == AXK_PAGE_STATE_AVAILABLE )
            {
                out_page_list[ ii++ ] = index;
                if( ii >= count ) { break; }
            }
        }

        // Check if not enough pages were found
        if( ii < count )
        {
            memset( out_page_list, 0, sizeof( uint64_t ) * count );
            axk_spinlock_release( &g_lock );

            return false;
        }
    }
    else
    {
        uint64_t real_range_begin   = b_prefer_high ? ( range_start - count ) + 1UL : range_start;
        uint64_t real_range_end     = real_range_begin + count;
        uint64_t ii                 = 0UL;

        for( uint64_t i = real_range_begin; i < real_range_end; i++ )
        {
            out_page_list[ ii++ ] = i;
        }
    }

    // 'out_page_list' is filled with the page numbers we want to allocate, so lets loop through and update the state of each page
    for( uint64_t i = 0; i < count; i++ )
    {
        uint64_t index = out_page_list[ i ];
        struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( index * 6UL ) );

        page_info->state        = AXK_PAGE_STATE_LOCKED;
        page_info->type         = type;
        page_info->process_id   = process_id;

        if( b_clear )
        {
            memset( (void*)( AXK_KERNEL_VA_PHYSICAL + ( index * AXK_PAGE_SIZE ) ), 0, AXK_PAGE_SIZE );
        }
    }

    axk_spinlock_release( &g_lock );
    return true;
}


bool axk_page_lock( uint64_t count, uint64_t* in_page_list, uint32_t process, uint8_t type, uint32_t flags )
{
    // Validate the parameters
    if( count == 0UL || in_page_list == NULL || process == AXK_PROCESS_INVALID ) { return false; }

    // Loop through the list of pages, and check if any are 'unlockable'
    axk_spinlock_acquire( &g_lock );

    for( uint64_t i = 0; i < count; i++ )
    {
        uint64_t index = in_page_list[ i ];
        if( index >= g_page_count ) { axk_spinlock_release( &g_lock ); return false; }

        struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( index * 6UL ) );

        if( page_info->state != AXK_PAGE_STATE_AVAILABLE )
        {
            axk_spinlock_release( &g_lock );
            return false;
        }
    }

    // Now, lets actually lock the pages
    for( uint64_t i = 0; i < count; i++ )
    {
        uint64_t index = in_page_list[ i ];
        struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( index * 6UL ) );

        if( page_info->state == AXK_PAGE_STATE_AVAILABLE )
        {
            page_info->state        = AXK_PAGE_STATE_LOCKED;
            page_info->process_id   = process;
            page_info->type         = type;
        }
    }

    // Release the lock
    axk_spinlock_release( &g_lock );
    return true;
}


bool axk_page_release( uint64_t count, uint64_t* in_page_list, uint32_t flags )
{
    // Validate parameters
    if( count == 0UL || in_page_list == NULL ) { return false; }

    // Check for flags that are relevant
    bool b_kernel = AXK_CHECK_FLAG( flags, AXK_PAGE_FLAG_KERNEL_REL );

    // Acquire lock
    axk_spinlock_acquire( &g_lock );

    // Check if all pages are able to be released first
    for( uint64_t i = 0; i < count; i++ )
    {
        uint64_t index = in_page_list[ i ];
        if( index >= g_page_count ) { axk_spinlock_release( &g_lock ); return false; }
        struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( index * 6UL ) );

        // If available or locked, then thats acceptable, but, if its anything else then we will fail
        if( page_info->state != AXK_PAGE_STATE_LOCKED && page_info->state != AXK_PAGE_STATE_AVAILABLE ) { axk_spinlock_release( &g_lock ); return false; }

        // If the page is a kernel page, and we dont have the 'AXK_PAGE_FLAG_KERNEL_REL' flag then fail
        if( page_info->process_id == AXK_PROCESS_KERNEL && !b_kernel ) { axk_spinlock_release( &g_lock ); return false; }
    }

    // Loop through each target page in the list, and unlock it
    for( uint64_t i = 0; i < count; i++ )
    {
        uint64_t index = in_page_list[ i ];
        struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( index * 6UL ) );

        if( page_info->state == AXK_PAGE_STATE_LOCKED )
        {
            page_info->state        = AXK_PAGE_STATE_AVAILABLE;
            page_info->type         = AXK_PAGE_TYPE_OTHER;
            page_info->process_id   = AXK_PROCESS_INVALID;
        }
    }

    // Release the lock
    axk_spinlock_release( &g_lock );
    return true;
}


bool axk_page_release_s( uint64_t count, uint64_t* in_page_list, uint32_t process, uint32_t flags )
{
    // Validate the parameters
    if( count == 0UL || in_page_list == NULL || process == AXK_PROCESS_INVALID ) { return false; }

    // Check for any relevant flags
    bool b_kernel = AXK_CHECK_FLAG( flags, AXK_PAGE_FLAG_KERNEL_REL );
    axk_spinlock_acquire( &g_lock );

    // Check if the page list is valid
    for( uint64_t i = 0; i < count; i++ )
    {
        uint64_t index = in_page_list[ i ];
        if( index >= g_page_count ) { axk_spinlock_release( &g_lock ); return false; }
        struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( index * 6UL ) );

        if( page_info->process_id == AXK_PROCESS_KERNEL && process != AXK_PROCESS_KERNEL && !b_kernel ) { axk_spinlock_release( &g_lock ); return false; }
        if( page_info->state == AXK_PAGE_STATE_LOCKED )
        {
            if( page_info->process_id != process ) { axk_spinlock_release( &g_lock ); return false; }
        }
        else if( page_info->state != AXK_PAGE_STATE_AVAILABLE )
        {
            axk_spinlock_release( &g_lock );
            return false;
        }
    }
    
    // So now, we can go through and actually release each page
    for( uint64_t i = 0; i < count; i++ )
    {
        uint64_t index = in_page_list[ i ];
        struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( index * 6UL ) );

        page_info->state        = AXK_PAGE_STATE_AVAILABLE;
        page_info->type         = AXK_PAGE_TYPE_OTHER;
        page_info->process_id   = AXK_PROCESS_INVALID;
    }

    axk_spinlock_release( &g_lock );
    return true;
}


bool axk_page_status( uint64_t in_page, uint32_t* out_process_id, uint8_t* out_state, uint8_t* out_type )
{
    // Get information about the page
    if( in_page >= g_page_count ) { return false; }
    struct axk_page_info_t* page_info = (struct axk_page_info_t*)( g_page_list + ( in_page * 6UL ) );
    
    // Acquire spinlock before reading anything
    axk_spinlock_acquire( &g_lock );

    if( out_process_id != NULL )    { *out_process_id = page_info->process_id; }
    if( out_state != NULL )         { *out_state = page_info->state; }
    if( out_type != NULL )          { *out_type = page_info->type; }
    
    axk_spinlock_release( &g_lock );
    return true;
}


bool axk_page_find( uint32_t target_process_id, uint64_t* out_count, uint64_t* out_page_list )
{
    return false;
}


uint64_t axk_page_count( void )
{
    return g_page_count;
}
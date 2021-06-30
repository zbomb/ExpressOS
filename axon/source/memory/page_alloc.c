/*==============================================================
    Axon Kernel - Page Allocator
    2021, Zachary Berry
    axon/source/memory/page_alloc.c

    * Contains definitions for functions in:
        axon/public/axon/memory/page_alloc.h
        axon/private/axon/memory/page_mgr.h
==============================================================*/

#include "axon/memory/page_mgr.h"
#include "axon/boot/boot_params.h"
#include "axon/library/spinlock.h"
#include "axon/panic.h"
#include "axon/config.h"
#include "axon/arch.h"
#include <string.h>

#define AXK_FLAG_PAGE_IN_USE    0x80000000
#define AXK_FLAG_PAGE_RESERVED  0x40000000
#define AXK_FLAG_PAGE_KERNEL    0x20000000

static uint64_t g_page_count = 0UL;
static uint64_t g_total_mem = 0UL;
static uint32_t* g_table = NULL;
static struct axk_spinlock_t g_lock;
static bool g_binit_called = false;


bool axk_pagemgr_init( void )
{
    // Ensure this isnt called more than once
    if( g_binit_called )
    {
        axk_panic( "PageManager: Init was called twice!" );
    }
    g_binit_called = true;

    // Get the memory map from the boot parameters 
    const struct axk_bootparams_memorymap_t* ptr_memmap = axk_bootparams_get_memorymap();

    g_total_mem         = ptr_memmap->total_mem;
    g_page_count        = ptr_memmap->entry_list[ ptr_memmap->entry_count - 1 ].end / AXK_PAGE_SIZE;

    // Debug message
    axk_terminal_prints( "Page Allocator: Available memory " );
    axk_terminal_printu64( ( g_total_mem / 1024UL / 1024UL ) );
    axk_terminal_prints( "MB \t(" );
    axk_terminal_printu64( g_page_count );
    axk_terminal_prints( " pages)\n" );

    // Scan the memory map, looking for a location in physical memory to store our page list
    uint64_t table_size     = g_page_count * sizeof( uint32_t );
    uint64_t table_offset   = 0UL;

    for( uint32_t i = 0; i < ptr_memmap->entry_count; i++ )
    {
        const struct axk_bootparams_memorymap_entry_t* ptr_entry = ptr_memmap->entry_list + i;

        // Ensure the address is in the lower 2GB of memory
        if( ptr_entry->begin + table_size >= 0x80000000UL )
        {
            return false;
        }

        // Ensure the table is placed on a 8-byte boundry
        uint64_t clean_begin = ptr_entry->begin;
        clean_begin += ( 8UL - ( clean_begin % 8UL ) );

        // Look for somewhere above 1MB
        if( ptr_entry->status == MEMORYMAP_ENTRY_STATUS_AVAILABLE && ( ptr_entry->end > table_size ) &&
            ( ptr_entry->end - table_size ) > 0x100000UL && ( ptr_entry->end - clean_begin ) >= table_size )
        {
            // We found a good section to write into!
            table_offset = clean_begin < 0x100000UL ? 0x100000UL : clean_begin;
            break;
        }
    }

    // Fail if we didnt find a location
    if( table_offset == 0UL ) { return false; }

    // TODO: If the table is outside of the 'AXK_KERNEL_VA_IMAGE' range (first 2GB of physical memory), then we will crash when scanning memory and writing
    // the table entries. The cleanest solution is to perform the virtual mappings first, not using this system
    // Instead, we simply store a list of the pages that contain page tables, and when we perform init here, we mark those pages properly with correct state
    //
    // e.x.
    // Map Init:
    // 1. Use the built-in tables to map the first 1GB of physical memory to AXK_KERNEL_VA_PHYSICAL
    // 2. Use the memory map to find available pages to use for page tables, continue mapping until complete
    // 3. Store the AXK_PAGE_ID for each page we wrote a page table to
    // 4. Pass this table to the axk_pagemgr_init function
    // 5. Then, during memory scanning, we can simply mark those pages as reserved
    //
    g_table = (uint32_t*)( table_offset + AXK_KERNEL_VA_IMAGE );

    // Loop through each page, figure out its state and write to the table
    for( uint64_t i = 0; i < g_page_count; i++ )
    {
        uint64_t page_begin     = i * AXK_PAGE_SIZE;
        uint64_t page_end       = page_begin + AXK_PAGE_SIZE;
        uint32_t page_info      = 0;

        // Find where this page is in the memory map
        for( uint32_t j = 0; j < ptr_memmap->entry_count; j++ )
        {
            const struct axk_bootparams_memorymap_entry_t* ptr_entry = ptr_memmap->entry_list + j;
            if( ( page_begin >= ptr_entry->begin && page_begin < ptr_entry->end ) ||
                ( page_end > ptr_entry->begin && page_end <= ptr_entry->end ) )
            {
                switch( ptr_entry->status )
                {
                    case MEMORYMAP_ENTRY_STATUS_KERNEL:
                    {
                        page_info = ( AXK_PROCESS_KERNEL | AXK_FLAG_PAGE_IN_USE | AXK_FLAG_PAGE_KERNEL | AXK_FLAG_PAGE_TYPE_IMAGE );
                        break;
                    }
                    case MEMORYMAP_ENTRY_STATUS_RESERVED:
                    {
                        page_info = ( AXK_FLAG_PAGE_IN_USE | AXK_FLAG_PAGE_RESERVED );
                        break;
                    }
                    case MEMORYMAP_ENTRY_STATUS_ACPI:
                    {
                        page_info = ( AXK_FLAG_PAGE_IN_USE | AXK_FLAG_PAGE_RESERVED | AXK_FLAG_PAGE_TYPE_ACPI );
                        break;
                    }
                    case MEMORYMAP_ENTRY_STATUS_RAMDISK:
                    {
                        page_info = ( AXK_FLAG_PAGE_IN_USE | AXK_FLAG_PAGE_RESERVED | AXK_FLAG_PAGE_TYPE_RAMDISK );
                        break;
                    }
                }
            }
        }

        // Write entry to memory
        g_table[ i ] = page_info;
    }

    // Now, we need to mark the pages we actually used to write the table
    for( uint64_t i = table_offset / AXK_PAGE_SIZE; i <= ( table_offset + table_size ) / AXK_PAGE_SIZE; i++ )
    {
        g_table[ i ] = ( AXK_FLAG_PAGE_IN_USE | AXK_FLAG_PAGE_RESERVED );
    }

    // We also want to reserve a single page for processor init code
    uint32_t ap_code_page = g_table[ AXK_AP_INIT_PAGE ];
    if( AXK_CHECK_ANY_FLAG( ap_code_page, AXK_FLAG_PAGE_IN_USE | AXK_FLAG_PAGE_RESERVED ) )
    {
        axk_panic( "Page Manager: Page that should be reserved for process init, is in use by the system" );
    }

    ap_code_page = ( AXK_FLAG_PAGE_IN_USE | AXK_FLAG_PAGE_RESERVED );
    g_table[ AXK_AP_INIT_PAGE ] = ap_code_page;

    // Setup the lock used to guard this system 
    axk_spinlock_init( &g_lock );    
    return true;
}


uint64_t axk_pagemgr_get_count( void )
{
    return g_page_count;
}


uint64_t axk_pagemgr_get_physmem( void )
{
    return g_total_mem;
}


void axk_pagemgr_reclaim( void )
{
    // Acquire the lock
    axk_spinlock_acquire( &g_lock );

    // Were going to find any pages that are able to be reclaimed (ACPI, initrd, Processor Init)
    // and free and clear them, so other processes can allocate those pages if need be
    for( uint64_t i = 0; i < g_page_count; i++ )
    {
        uint32_t page_info = g_table[ i ];
        if( AXK_CHECK_ANY_FLAG( page_info, AXK_FLAG_PAGE_TYPE_ACPI | AXK_FLAG_PAGE_TYPE_RAMDISK ) )
        {
            g_table[ i ] = 0;
        }
    }

    g_table[ AXK_AP_INIT_PAGE ] = 0;

    // Release the lock
    axk_spinlock_release( &g_lock );
}


bool axk_page_acquire( uint64_t count, AXK_PAGE_ID* out_list, uint32_t process, AXK_PAGE_FLAGS flags )
{
    // Validate the parameters
    if( count == 0UL || out_list == NULL || process == AXK_PROCESS_INVALID ) { return false; }

    // Acquire the lock
    axk_spinlock_acquire( &g_lock );

    // We need to find 'count' number of pages and lock them for the calling process
    uint32_t clean_flags    = ( flags & 0x1C000000U );
    bool b_prefhigh         = AXK_CHECK_FLAG( flags, AXK_FLAG_PAGE_PREFER_HIGH );
    bool b_clear            = AXK_CHECK_FLAG( flags, AXK_FLAG_PAGE_CLEAR_ON_LOCK );
    
    uint64_t index = 0UL;

    if( b_prefhigh )
    {
        for( uint64_t i = g_page_count - 1UL; i >= i; i-- )
        {
            if( !AXK_CHECK_ANY_FLAG( g_table[ i ], AXK_FLAG_PAGE_IN_USE | AXK_FLAG_PAGE_RESERVED ) )
            {
                out_list[ index++ ] = i;
                if( index == count ) { break; }
            }
        }
    }
    else
    {
        for( uint64_t i = 1; i < g_page_count; i++ )
        {
            if( !AXK_CHECK_ANY_FLAG( g_table[ i ], AXK_FLAG_PAGE_IN_USE | AXK_FLAG_PAGE_RESERVED ) )
            {
                out_list[ index++ ] = i;
                if( index == count ) { break; }
            }
        }
    }

    // Check to see if we have all the pages requested
    if( index != count )
    {
        // TODO: In the future, we will have to call a function to find un-used pages and dump them
        axk_spinlock_release( &g_lock );
        return false;
    }

    // Now, we need to loop through the pages we are locking, and update their state
    uint32_t page_info = ( process | clean_flags | AXK_FLAG_PAGE_IN_USE );
    if( process == AXK_PROCESS_KERNEL ) { page_info |= AXK_FLAG_PAGE_KERNEL; }

    for( uint64_t i = 0; i < count; i++ )
    {
        g_table[ out_list[ i ] ] = page_info;
    }

    // Release the lock
    axk_spinlock_release( &g_lock );

    // Check if we need to clear the pages
    if( b_clear )
    {
        for( uint64_t i = 0; i < count; i++ )
        {
            uint64_t page_begin = out_list[ i ] * AXK_PAGE_SIZE;
            memset( (void*) page_begin + AXK_KERNEL_VA_PHYSICAL, 0, AXK_PAGE_SIZE );
        }
    }

    return true;
}


bool axk_page_lock( uint64_t count, AXK_PAGE_ID* in_list, uint32_t process, AXK_PAGE_FLAGS flags )
{
    // Validate the parameters
    if( count == 0UL || in_list == NULL || process == AXK_PROCESS_INVALID ) { return false; }

    // Acquire the lock
    axk_spinlock_acquire( &g_lock );

    // Loop through the pages, validating the whole request before attempting to lock any pages at all
    for( uint64_t i = 0; i < count; i++ )
    {
        uint64_t page_index = in_list[ i ];
        if( page_index >= g_page_count || page_index == 0 )
        {
            axk_spinlock_release( &g_lock );
            return false;
        }

        uint32_t page_info = g_table[ page_index ];
        if( AXK_CHECK_ANY_FLAG( page_info, AXK_FLAG_PAGE_IN_USE | AXK_FLAG_PAGE_RESERVED ) )
        {
            axk_spinlock_release( &g_lock );
            return false;
        }
    }

    // Build the page list entry
    uint32_t clean_flags    = ( flags & 0x1C000000U );
    bool b_clear            = AXK_CHECK_FLAG( flags, AXK_FLAG_PAGE_CLEAR_ON_LOCK );

    uint32_t new_page_info = ( process | AXK_FLAG_PAGE_IN_USE | clean_flags );
    if( process == AXK_PROCESS_KERNEL ) { new_page_info |= AXK_FLAG_PAGE_KERNEL; }

    // Loop through and set each entry
    for( uint64_t i = 0; i < count; i++ )
    {
        g_table[ in_list[ i ] ] = new_page_info;
    }

    // Release the lock
    axk_spinlock_release( &g_lock );

    // Clear the pages if requested
    if( b_clear )
    {
        for( uint64_t i = 0; i < count; i++ )
        {
            uint64_t page_begin = in_list[ i ] * AXK_PAGE_SIZE;
            memset( (void*) page_begin + AXK_KERNEL_VA_PHYSICAL, 0, AXK_PAGE_SIZE );
        }
    }

    return true;
}


bool axk_page_release( uint64_t count, AXK_PAGE_ID* in_list, bool b_allowkernel )
{
    // Validate parameters
    if( count == 0UL || in_list == NULL ) { return false; }

    // Acquire lock
    axk_spinlock_acquire( &g_lock );

    // Loop through each page and check if its able to be released
    // We dont actually want to perform any action until its validated
    for( uint64_t i = 0; i < count; i++ )
    {
        uint64_t page_index = in_list[ i ];
        if( page_index == 0UL || page_index >= g_page_count )
        {
            axk_spinlock_release( &g_lock );
            return false;
        }

        uint32_t page_info = g_table[ page_index ];
        
        // Skip pages that arent in-use
        if( AXK_CHECK_FLAG( page_info, AXK_FLAG_PAGE_IN_USE ) )
        {
            // Disallow releasing reserved pages, kernel image pages, or any kernel pages when 'b_allowkernel' is false
            if( AXK_CHECK_FLAG( page_info, AXK_FLAG_PAGE_RESERVED ) || 
                AXK_CHECK_FLAG( page_info, AXK_FLAG_PAGE_KERNEL | AXK_FLAG_PAGE_TYPE_IMAGE ) ||
                ( AXK_CHECK_FLAG( page_info, AXK_FLAG_PAGE_KERNEL ) && !b_allowkernel ) )
            {
                axk_spinlock_release( &g_lock );
                return false;
            }
        }
    }

    // All pages were okay to be released, so lets do it now
    for( uint64_t i = 0; i < count; i++ )
    {
        g_table[ in_list[ i ] ] = 0U;
    }

    // Release lock
    axk_spinlock_release( &g_lock );

    return true;
}


bool axk_page_status( AXK_PAGE_ID page_id, uint32_t* out_process, AXK_PAGE_FLAGS* out_flags )
{
    // Validate the parameters
    if( page_id == 0UL || page_id >= g_page_count || out_process == NULL || out_flags == NULL ) { return false; }

    // Acquire lock
    axk_spinlock_acquire( &g_lock );

    // Find the target page, copy info into the return pointers
    uint32_t page_info = g_table[ page_id ];

    // Release the lock first
    axk_spinlock_release( &g_lock );

    *out_process     = ( page_info & 0x00FFFFFF );
    *out_flags       = ( page_info & 0xFF000000 );
    
    return true;
}


uint64_t axk_page_freeproc( uint32_t process )
{
    // Validate the parameter
    if( process == AXK_PROCESS_INVALID || process == AXK_PROCESS_KERNEL || ( process & 0x00FFFFFF ) != process ) { return false; }

    // Acquire lock
    axk_spinlock_acquire( &g_lock );

    // Loop through each page, look for ones owned by the process and release all of them
    uint64_t counter = 0UL;

    for( uint64_t i = 1; i < g_page_count; i++ )
    {
        uint32_t page_info = g_table[ i ];
        if( ( page_info & 0x00FFFFFF ) == process )
        {
            // Although neither should be set, lets just do a sanity check
            if( AXK_CHECK_ANY_FLAG( page_info, AXK_FLAG_PAGE_RESERVED | AXK_FLAG_PAGE_KERNEL ) )
            {
                axk_panic( "Page Manager: a page that was owned by a process.. was marked as 'reserved' or 'kernel'" );
            }

            g_table[ i ] = 0U;
            counter++;
        }
    }

    // Release the spinlock
    axk_spinlock_release( &g_lock );
    return counter;
}
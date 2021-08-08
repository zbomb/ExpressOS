/*==============================================================
    Axon Kernel - Kernel Heap
    2021, Zachary Berry
    axon/source/memory/kheap.c
==============================================================*/

#include "axon/boot/basic_out.h"
#include "axon/memory/kheap_private.h"
#include "axon/memory/kheap.h"
#include "axon/memory/page_alloc.h"
#include "axon/memory/kmap.h"
#include "axon/library/spinlock.h"
#include "axon/panic.h"
#include <string.h>

/*
    Private Structure/Constants
*/
struct axk_kheap_tag
{
    uint64_t next_entry;
    uint64_t prev_entry;

} __attribute__((__packed__));

#define AXK_FLAG_KHEAP_NONE     0x00
#define AXK_FLAG_KHEAP_PRESENT  0x01
#define AXK_FLAG_KHEAP_BEGIN    0x02
#define AXK_KHEAP_VALID_PREV    0xFA00000000000000UL
#define AXK_KHEAP_VALID_NEXT    0xAF0000000000000UL

/*
    State
*/
static struct axk_spinlock_t g_lock;
static struct axk_kheap_tag* g_lowest_tag = NULL;
static struct axk_kheap_tag* g_highest_tag = NULL;
static uint64_t g_kheap_pages = 0UL;
static bool g_init = false;

/*
    Function Implementations
*/
bool axk_kheap_init( void )
{
    // Guard against double-init
    if( g_init )
    {
        axk_panic( "Kernel Heap: attempt to double initialize" );
    }
    g_init = true;

    // Allocate a starting page
    AXK_PAGE_ID phys_page;
    if( !axk_page_acquire( 1UL, &phys_page, AXK_PROCESS_KERNEL, AXK_FLAG_PAGE_TYPE_HEAP ) )
    {
        return false;
    }

    if( !axk_kmap( phys_page, AXK_KERNEL_VA_HEAP, AXK_FLAG_MAP_ALLOW_OVERWRITE ) )
    {
        return false;
    }

    // Write our 'empty entry' begin tag for the heap
    struct axk_kheap_tag* begin_tag = (struct axk_kheap_tag*)( AXK_KERNEL_VA_HEAP );
    begin_tag->next_entry = AXK_KHEAP_VALID_NEXT | AXK_FLAG_KHEAP_BEGIN;
    begin_tag->prev_entry = AXK_KHEAP_VALID_PREV;

    // Setup state
    g_kheap_pages   = 1UL;
    g_lowest_tag    = begin_tag;
    g_highest_tag   = begin_tag;

    axk_spinlock_init( &g_lock );
    return true;
}


uint64_t axk_kheap_page_count( void )
{
    return g_kheap_pages;
}


bool alloc_helper_expand( uint64_t sz, struct axk_kheap_tag* end_tag )
{
    uint64_t end_last   = ( ( (uint64_t)end_tag / AXK_PAGE_SIZE ) * AXK_PAGE_SIZE ) + AXK_PAGE_SIZE;
    uint64_t remaining  = end_last - (uint64_t)end_tag - 0x10UL;

    // Allocate the required pages
    if( sz + 0x10UL > remaining )
    {
        uint64_t new_space      = sz + 0x10UL - remaining;
        uint64_t needed_pages   = new_space / AXK_PAGE_SIZE;
        if( ( new_space % AXK_PAGE_SIZE ) != 0UL ) { needed_pages++; }

        // Allocate the needed number of pages and map
        for( uint64_t i = 0; i < needed_pages; i++ )
        {
            AXK_PAGE_ID phys_page;
            if( !axk_page_acquire( 1UL, &phys_page, AXK_PROCESS_KERNEL, AXK_FLAG_PAGE_TYPE_PTABLE ) )
            {
                axk_panic( "Kernel Heap: failed to allocate page for heap" );
            }

            if( !axk_kmap( phys_page, end_last + ( i * AXK_PAGE_SIZE ), AXK_FLAG_MAP_ALLOW_OVERWRITE ) )
            {
                axk_panic( "Kernel Heap: failed to map page needed for kernel heap" );
            }

            // Increment our page counter
            g_kheap_pages++;
        }
    }

    // Write the 'end tag'
    struct axk_kheap_tag* ptr_end = (struct axk_kheap_tag*)( ( (uint8_t*) end_tag ) + sz + 0x10UL );

    ptr_end->next_entry     = AXK_KHEAP_VALID_NEXT;
    ptr_end->prev_entry     = ( ( (uint64_t) end_tag - AXK_KERNEL_VA_HEAP ) | AXK_KHEAP_VALID_PREV );
    g_highest_tag           = ptr_end;

    // Update the current entry tag, preserve begin tag if needed
    end_tag->next_entry = ( ( (uint64_t)ptr_end - AXK_KERNEL_VA_HEAP ) | AXK_KHEAP_VALID_NEXT | ( end_tag->next_entry & AXK_FLAG_KHEAP_BEGIN ) );
    return true;
}


bool alloc_helper_avail( uint64_t sz, struct axk_kheap_tag* in_tag )
{
    // Calculate available size
    uint64_t next_addr      = ( in_tag->next_entry & 0x0000FFFFFFFFFFF0UL ) + AXK_KERNEL_VA_HEAP;
    uint64_t total_space    = next_addr - ( (uint64_t)in_tag + 0x10UL );

    if( total_space < sz ) { return false; }

    bool b_split            = ( total_space > sz + AXK_KHEAP_MIN_ALLOC );
    uint64_t needed_space   = b_split ? sz + sizeof( struct axk_kheap_tag ) : sz;

    // Calculate how far we need to get past the end of the page
    uint64_t page_end       = ( ( (uint64_t)in_tag ) / AXK_PAGE_SIZE ) * AXK_PAGE_SIZE + AXK_PAGE_SIZE;
    uint64_t needed_end     = (uint64_t)in_tag + needed_space;

    // Check if the new entry will fit on this page
    if( needed_end > page_end )
    {
        // Calculate how far past the end of the page were going
        uint64_t bytes_past = ( (uint64_t)in_tag + needed_space ) - page_end;
        uint64_t pages_past = ( bytes_past / AXK_PAGE_SIZE ) + ( ( bytes_past % AXK_PAGE_SIZE ) > 0UL ? 1UL : 0UL );

        // Check if the tag falls on the last page
        uint64_t begin_last_page = page_end + ( ( pages_past - 1UL ) * AXK_PAGE_SIZE );
        if( next_addr >= begin_last_page && next_addr < ( begin_last_page + AXK_PAGE_SIZE ) )
        {
            // If so, we dont need to allocate this page
            pages_past--;
        }

        // Map needed memory
        for( uint64_t i = 0; i < pages_past; i++ )
        {
            AXK_PAGE_ID page_id;
            if( !axk_page_acquire( 1UL, &page_id, AXK_PROCESS_KERNEL, AXK_FLAG_PAGE_CLEAR_ON_LOCK | AXK_FLAG_PAGE_TYPE_HEAP ) ||
                !axk_kmap( page_id, page_end + ( i * AXK_PAGE_SIZE ), AXK_FLAG_NONE ) )
            {
                axk_panic( "Kernel Heap: failed to map memory needed to expand the heap" );
            }

            g_kheap_pages++;
        }
    }

    // Memory is mapped in, check if were splitting into two entries, and write new tag for it
    if( b_split )
    {
        struct axk_kheap_tag* new_tag = (struct axk_kheap_tag*)( (uint64_t)in_tag + needed_space );
        
        new_tag->next_entry     = ( in_tag->next_entry & 0x0000FFFFFFFFFFF0UL ) | AXK_KHEAP_VALID_NEXT;
        new_tag->prev_entry     = ( (uint64_t)in_tag - AXK_KERNEL_VA_HEAP ) | AXK_KHEAP_VALID_PREV;
        in_tag->next_entry      = ( (uint64_t)new_tag - AXK_KERNEL_VA_HEAP ) | AXK_KHEAP_VALID_NEXT;
    }

    return true;
}


void* axk_kheap_alloc( size_t sz, bool b_clear )
{
    // Validate parameters
    if( sz == 0UL ) { return NULL; }

    // Fix requested size to be aligned
    sz += ( AXK_KHEAP_ALIGN - ( sz % AXK_KHEAP_ALIGN ) );

    // Acquire lock
    axk_spinlock_acquire( &g_lock );

    const uint8_t* heap_begin   = (uint8_t*)( AXK_KERNEL_VA_HEAP );
    const uint8_t* heap_end     = (uint8_t*)( AXK_KERNEL_VA_SHARED );

    uint8_t* pos        = (uint8_t*)( g_lowest_tag );
    bool b_movelowtag   = false;

    // Look for somewhere to place the memory request
    while( pos < heap_end )
    {
        struct axk_kheap_tag* ptr_tag = (struct axk_kheap_tag*)( pos );
        uint64_t offset_tag = ( (uint64_t)pos ) - AXK_KERNEL_VA_HEAP;

        // Optional: Validate heap tag
        if( AXK_KHEAP_VALIDATE && (
            !AXK_CHECK_FLAG( ptr_tag->prev_entry, AXK_KHEAP_VALID_PREV ) ||
            !AXK_CHECK_FLAG( ptr_tag->next_entry, AXK_KHEAP_VALID_NEXT ) ) )
        {
            axk_panic( "Kernel Heap: possible corruption detected" );
        }

        // Check if the current entry is free
        if( !AXK_CHECK_ANY_FLAG( ptr_tag->next_entry, AXK_FLAG_KHEAP_PRESENT ) )
        {
            // Check if were at the end of the heap
            bool b_avail = false;
            if( ( ptr_tag->next_entry & 0x0000FFFFFFFFFFF0UL ) == 0UL )
            {

                if( !alloc_helper_expand( sz, ptr_tag ) )
                {
                    axk_panic( "Kernel Heap: failed to expand the heap" );
                }

                b_avail = true;
            }
            else
            {
                if( !alloc_helper_avail( sz, ptr_tag ) )
                {
                    b_avail = false;
                }
            }

            if( b_avail )
            {
                ptr_tag->next_entry |= AXK_FLAG_KHEAP_PRESENT;
                if( g_lowest_tag == ptr_tag || b_movelowtag )
                {
                    g_lowest_tag = (struct axk_kheap_tag*)( ( ptr_tag->next_entry & 0x0000FFFFFFFFFFF0UL ) + AXK_KERNEL_VA_HEAP );
                }

                axk_spinlock_release( &g_lock );

                // Clear the memory if requested
                // TODO: Track if physical pages are cleared or not, so we can conditionally skip this
                void* ret_ptr = (void*)( ( (uint64_t)ptr_tag ) + sizeof( struct axk_kheap_tag ) );
                if( b_clear )
                {
                    memset( ret_ptr, 0, sz );
                }

                return ret_ptr;
            }
        }
        else if( (uint64_t)pos == (uint64_t)g_lowest_tag )
        {
            b_movelowtag = true;
        }

        // Check next tag in the heap
        pos = (uint8_t*)( ( ptr_tag->next_entry & 0x0000FFFFFFFFFFF0UL ) + AXK_KERNEL_VA_HEAP );
    }

    axk_panic( "Kernel Heap: ran out of heap space!" );
    return NULL;
}


void* axk_kheap_realloc( void* ptr, size_t new_sz, bool b_clear )
{
    // To do this, we can follow these steps....
    // 1. Check if the new size is smaller or larger...
    // If smaller:
    //  1a. Check if next tag is not present, if not, then move its begin tag back, update current begin tag, and next end tag
    //  1b. If it is present, then check if the size difference is enough to split the current entry
    //  1c. If we are splitting, then create a new entry, update current and next entry tags
    //  1d. If we arent splitting, simply return without changing anything
    //
    // If larger:
    //  a. Check if the next entry is empty, and if we can consume it to make a single entry that is large enough
    //  b. If its not empty, and we cant grow then release the current entry and allocate a new entry

    // TEMP: For now, were just going to accomplish this by getting a new block
    if( ptr == NULL ) { return NULL; }
    axk_kheap_free( ptr );

    if( new_sz == 0UL ) { return NULL; }
    return axk_kheap_alloc( new_sz, b_clear );
}


void axk_kheap_free( void* ptr )
{
    // If pointer is null, do nothing
    if( ptr == NULL || (uint64_t)ptr == 0x00UL ) { return; }

    // If the pointer is outside of the heap address space, crash
    if( (uint64_t)ptr < ( AXK_KERNEL_VA_HEAP + sizeof( struct axk_kheap_tag) ) || (uint64_t)ptr >= AXK_KERNEL_VA_SHARED )
    {
        axk_panic( "Kernel Heap: attempt to free memory outside of the heap address range" );
    }

    // Acquire lock
    axk_spinlock_acquire( &g_lock );

    // Read tag for the target entry and validate it
    struct axk_kheap_tag* ptr_tag = (struct axk_kheap_tag*)( (uint64_t)ptr - sizeof( struct axk_kheap_tag) );
    if( !AXK_CHECK_FLAG( ptr_tag->next_entry, AXK_KHEAP_VALID_NEXT ) ||
        !AXK_CHECK_FLAG( ptr_tag->prev_entry, AXK_KHEAP_VALID_PREV ) ||
        ( ptr_tag->next_entry & 0x0000FFFFFFFFFFF0UL ) == 0UL )
    {
        axk_panic( "Kernel Heap: attempt to free with an invalid address" );
    }

    // Check if there is a previous or next entry in the heap that is not present
    struct axk_kheap_tag* ptr_next = NULL;
    struct axk_kheap_tag* ptr_prev = NULL;

    if( !AXK_CHECK_FLAG( ptr_tag->prev_entry, AXK_FLAG_KHEAP_BEGIN ) )
    {
        // Were not the first heap entry, so there should be a previous entry
        ptr_prev = (struct axk_kheap_tag*)( ( ptr_tag->prev_entry & 0x0000FFFFFFFFFFF0UL ) + AXK_KERNEL_VA_HEAP );

        // If the previous entry is in-use, then ignore it
        if( AXK_CHECK_FLAG( ptr_prev->next_entry, AXK_FLAG_KHEAP_PRESENT ) )
        {
            ptr_prev = NULL;
        }
    }

    bool b_end = false;
    ptr_next = (struct axk_kheap_tag*)( ( ptr_tag->next_entry & 0x0000FFFFFFFFFFF0UL ) + AXK_KERNEL_VA_HEAP );
    if( ( ptr_next->next_entry & 0x0000FFFFFFFFFFF0UL ) == 0UL )
    {
        // Found the end of the heap!
        b_end = true;
        ptr_next = NULL;
    }
    else if( AXK_CHECK_FLAG( ptr_next->next_entry, AXK_FLAG_KHEAP_PRESENT ) )
    {
        // Ignore present entry
        ptr_next = NULL;
    }

    // If either the next or previous entry was not present, were going to combine with the current entry into one big empty entry
    struct axk_kheap_tag* begin_tag     = ( ptr_prev != NULL ? ptr_prev : ptr_tag );
    struct axk_kheap_tag* end_tag       = (struct axk_kheap_tag*)( ( ( ptr_next != NULL ? ptr_next->next_entry : ptr_tag->next_entry ) & 0x0000FFFFFFFFFFF0UL ) + AXK_KERNEL_VA_HEAP );

    // Set these tags up to point to each other, preserve begin flag if begin tag has it
    begin_tag->next_entry = ( ( (uint64_t)end_tag - AXK_KERNEL_VA_HEAP ) | AXK_KHEAP_VALID_NEXT | ( begin_tag->next_entry & AXK_FLAG_KHEAP_BEGIN ) );
    end_tag->prev_entry = ( ( (uint64_t)begin_tag - AXK_KERNEL_VA_HEAP ) | AXK_KHEAP_VALID_PREV );

    // Calculate the pages the begin and end tags are on
    uint64_t begin_page     = ( (uint64_t)begin_tag / AXK_PAGE_SIZE ) * AXK_PAGE_SIZE;
    uint64_t end_page       = ( (uint64_t)end_tag / AXK_PAGE_SIZE ) * AXK_PAGE_SIZE;

    // Unmap any pages that arent needed anymore
    for( uint64_t i = begin_page + AXK_PAGE_SIZE; i < end_page; i += AXK_PAGE_SIZE )
    {
        AXK_PAGE_ID page_id = axk_kunmap( i );
        if( page_id != 0UL )
        {
            if( !axk_page_release( 1UL, &page_id, true ) )
            {
                axk_panic( "Kernel Heap: failed to release unused page" );
            }

            g_kheap_pages--;
        }
    }

    // Determine if end tag is on a different page than the begin tag so we can free the page the end tag is on
    if( b_end )
    {
        if( end_page > begin_page )
        {
            AXK_PAGE_ID page_id = axk_kunmap( end_page );
            if( page_id != 0UL )
            {
                if( !axk_page_release( 1UL, &page_id, true ) )
                {
                    axk_panic( "Kernel Heap: failed to release unused page" );
                }

                g_kheap_pages--;
            }
        }

        // Add flags to the new 'end of heap' tag
        begin_tag->next_entry = AXK_KHEAP_VALID_NEXT | ( begin_tag->next_entry & AXK_FLAG_KHEAP_BEGIN );
        g_highest_tag = begin_tag;
    }

    // Update global 'lowest tag'
    g_lowest_tag = begin_tag < g_lowest_tag ? begin_tag : g_lowest_tag;

    // Release lock
    axk_spinlock_release( &g_lock );
}


void axk_kheap_debug( void )
{
    // Acquire spin-lock
    axk_spinlock_acquire( &g_lock );

    // Print header
    axk_basicout_lock();
    axk_basicout_prints( "===================> Heap State <=====================\n" );

    // Start with the first heap tag
    struct axk_kheap_tag* ptr_iter = (struct axk_kheap_tag*) AXK_KERNEL_VA_HEAP;
    while( ptr_iter <= g_highest_tag )
    {
        // Print info about this entry
        uint64_t next = ( ptr_iter->next_entry & 0x0000FFFFFFFFFFF0UL ) + AXK_KERNEL_VA_HEAP;
        uint64_t prev = ( ptr_iter->prev_entry & 0x0000FFFFFFFFFFF0UL ) + AXK_KERNEL_VA_HEAP;

        bool b_present  = AXK_CHECK_FLAG( ptr_iter->next_entry, AXK_FLAG_KHEAP_PRESENT );
        bool b_begin    = AXK_CHECK_FLAG( ptr_iter->next_entry, AXK_FLAG_KHEAP_BEGIN );
        bool b_end      = ( ( ptr_iter->next_entry & 0x0000FFFFFFFFFFF0UL ) == 0UL );

        axk_basicout_printtab();
        axk_basicout_printh64( (uint64_t)ptr_iter, true );
        axk_basicout_prints( ":\n\t\tPresent: " );
        axk_basicout_prints( b_present ? "YES" : "NO" );
        axk_basicout_prints( "  Begin: " );
        axk_basicout_prints( b_begin ? "YES" : "NO" );
        axk_basicout_prints( "  End: " );
        axk_basicout_prints( b_end ? "YES" : "NO" );
        axk_basicout_prints( "\n\t\tNext: " );
        axk_basicout_printh64( next, false );
        axk_basicout_prints( " Prev: " );
        axk_basicout_printh64( prev, false );
        axk_basicout_printnl();
        
        // Check for the end of the heap
        if( next == AXK_KERNEL_VA_HEAP )
        {
            axk_basicout_prints( "=================> Heap End <=================\n" );
            break;
        }

        ptr_iter = (struct axk_kheap_tag*)( next );
    }

    // Print additional state
    axk_basicout_prints( "\n\tPages: " );
    axk_basicout_printu64( g_kheap_pages );
    axk_basicout_prints( "  Lowest Avail: " );
    axk_basicout_printh64( (uint64_t) g_lowest_tag, false );
    axk_basicout_printnl();
    axk_basicout_unlock();

    // Release lock
    axk_spinlock_release( &g_lock );
}


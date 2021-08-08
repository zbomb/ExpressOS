/*==============================================================
    Axon Kernel - Memory Map Manager (X86)
    2021, Zachary Berry
    axon/source/arch_x86/kmap.c

    * Contains definitions for functions in:
        axon/public/axon/memory/kmap.h
        axon/private/axon/memory/kmap_private.h
==============================================================*/

#ifdef _X86_64_
#include "axon/panic.h"
#include "axon/memory/kmap.h"
#include "axon/memory/kmap_private.h"
#include "axon/memory/page_alloc_private.h"
#include "axon/memory/page_alloc.h"
#include "axon/library/spinlock.h"
#include "axon/memory/atomics.h"

/*
    State
*/
static uint64_t g_ptr_kpml4 = 0UL;
static struct axk_spinlock_t g_lock;
static bool g_init = false;
static struct axk_atomic_uint64_t g_shared_counter;

/*
    ASM externs
*/
extern uint8_t axk_pml4;
extern uint8_t axk_pdpt_low;
extern uint8_t axk_pdt_low;


bool axk_mapmgr_init( void )
{
    // Guard against double init
    if( g_init )
    {
        axk_panic( "Memory Map Manager: attempted double initialize!" );
    }
    g_init = true;

    axk_atomic_store_uint64( &g_shared_counter, 0, MEMORY_ORDER_SEQ_CST );

    // Determine the number of huge pages needed to map all of the physical pages
    uint64_t huge_count = axk_pagemgr_get_count() / 512UL;
    if( axk_pagemgr_get_count() % 512UL != 0UL ) { huge_count++; }

    // Get addresses of the page tables stored in the kernel image
    g_ptr_kpml4         = (uint64_t) &axk_pml4;
    uint64_t ptr_kpdpt  = (uint64_t) &axk_pdpt_low;
    uint64_t ptr_kpdt   = (uint64_t) &axk_pdt_low;

    // Write the first 1GB of mappings to AXK_KERNEL_VA_PHYSICAL
    uint64_t* pml4_array    = (uint64_t*) g_ptr_kpml4;
    uint64_t* pdpt_array    = (uint64_t*) ptr_kpdpt;
    uint64_t* pdt_array     = (uint64_t*) ptr_kpdt;
    uint64_t pml4_entries   = 1UL;
    uint64_t pdpt_entries   = 1UL;
    uint64_t pdt_entries    = 0UL;
    uint64_t page_counter   = 512UL;

    pml4_array[ 256 ]   = ( ( ( ptr_kpdpt - AXK_KERNEL_VA_IMAGE ) & 0xFFFFFFFFFF000UL ) | AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_WRITABLE );
    pdpt_array[ 0 ]     = ( ( ( ptr_kpdt - AXK_KERNEL_VA_IMAGE ) & 0xFFFFFFFFFF000UL ) | AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_WRITABLE );

    for( uint64_t i = 0; i < 512; i++ )
    {
        pdt_array[ i ] = ( ( i * 0x200000UL ) | AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_WRITABLE | AXK_FLAG_PAGEMAP_HUGE_PAGE );
        pdt_entries++;
    }

    // Now, map the remaining physical memory
    // We might not create 512 PDTs here, but its the max before we exhaust the PDPT in the kernel image
    for( uint64_t i = 1; i < 512UL; i++ )
    {
        // Acquire a page
        AXK_PAGE_ID phys_page;
        if( !axk_page_acquire( 1, &phys_page, AXK_PROCESS_KERNEL, AXK_FLAG_PAGE_TYPE_PTABLE | AXK_FLAG_PAGE_CLEAR_ON_LOCK ) )
        {
            return false;
        }

        // Write a PDPT entry pointing to this page
        pdpt_array[ i ] = ( ( phys_page * AXK_PAGE_SIZE ) | AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_WRITABLE );
        pdpt_entries++;

        // Write data to our newly allocated PDT
        uint64_t* new_pdt_array = (uint64_t*)( ( phys_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );
        for( uint32_t j = 0; j < 512; j++ )
        {
            new_pdt_array[ j ] = ( ( page_counter * 0x200000UL ) | AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_WRITABLE | AXK_FLAG_PAGEMAP_HUGE_PAGE );
            page_counter++;
            pdt_entries++;

            // Check if we are done mapping
            if( page_counter >= huge_count ) { break; }
        }

        if( page_counter >= huge_count ) { break; }
    }

    // Check if there are any pages left to be mapped
    if( page_counter < huge_count )
    {
        // At this point, we have filled a whole PML4 entry (512GB)
        // Since this space allows for 64TB of mappings, we can have a total of 127 additional PML4 entries
        for( uint64_t i = 1; i < 127; i++ )
        {
            // Allocate a physical page for a new PDPT
            AXK_PAGE_ID pdpt_page;
            if( !axk_page_acquire( 1UL, &pdpt_page, AXK_PROCESS_KERNEL, AXK_FLAG_PAGE_CLEAR_ON_LOCK | AXK_FLAG_PAGE_TYPE_PTABLE ) )
            {
                return false;
            }

            uint64_t* new_pdpt_array = (uint64_t*)( ( pdpt_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );

            // Write PML4 entry for this new PDPT
            pml4_array[ i ] = ( ( pdpt_page * AXK_PAGE_SIZE ) | AXK_FLAG_PAGEMAP_WRITABLE | AXK_FLAG_PAGEMAP_PRESENT );
            pml4_entries++;

            // Fill the new PDPT
            for( uint64_t j = 0; j < 512UL; j++ )
            {
                // Allocate a new PDT
                AXK_PAGE_ID pdt_page;
                if( !axk_page_acquire( 1UL, &pdt_page, AXK_PROCESS_KERNEL, AXK_FLAG_PAGE_CLEAR_ON_LOCK | AXK_FLAG_PAGE_TYPE_PTABLE ) )
                {
                    return false;
                }

                uint64_t* new_pdt_array = (uint64_t*)( ( pdt_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );

                // Write PDPT entry for this new PDT
                new_pdpt_array[ j ] = ( ( pdt_page * AXK_PAGE_SIZE ) | AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_WRITABLE );
                pdpt_entries++;

                // Fill the new PDT
                for( uint64_t k = 0; k < 512UL; k++ )
                {
                    new_pdt_array[ k ] = ( ( page_counter * 0x200000UL ) | AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_WRITABLE | AXK_FLAG_PAGEMAP_HUGE_PAGE );
                    pdt_entries++;
                    page_counter++;

                    if( page_counter >= huge_count ) { break; }
                }

                if( page_counter >= huge_count ) { break; }
            }

            if( page_counter >= huge_count ) { break; }
        }
    }

    // Print debug message
    axk_basicout_prints( "Memory Map: Physical address range " );
    axk_basicout_printh64( AXK_KERNEL_VA_PHYSICAL, false );
    axk_basicout_prints( " to " );
    axk_basicout_printh64( AXK_KERNEL_VA_PHYSICAL + ( page_counter * 0x200000UL ), false );
    axk_basicout_printnl();
    
    // Initialize the spinlock
    axk_spinlock_init( &g_lock );
    return true;
}


void* axk_mapmgr_get_table( void )
{
    return (void*) g_ptr_kpml4;
}


bool axk_kmap( AXK_PAGE_ID page, uint64_t virt_addr, AXK_MAP_FLAGS flags )
{
    // Validate the parameters
    if( page == 0UL || virt_addr < AXK_KERNEL_VA_HEAP || virt_addr >= AXK_KERNEL_VA_IMAGE || ( virt_addr % AXK_PAGE_SIZE ) != 0UL ) { return false; }

    // Check for 'allow overwrite' flag
    bool b_overwrite = AXK_CHECK_ANY_FLAG( flags, AXK_FLAG_MAP_ALLOW_OVERWRITE );
    flags = ( flags & 0x8000000000000FFFUL );

    // Calculate indicies
    uint32_t pml4_index  = (uint32_t)( ( virt_addr & 0x0000FF8000000000UL ) >> 39 );
    uint32_t pdpt_index  = (uint32_t)( ( virt_addr & 0x0000007FC0000000UL ) >> 30 );
    uint32_t pdt_index   = (uint32_t)( ( virt_addr & 0x000000003FE00000UL ) >> 21 );
    uint32_t pt_index    = (uint32_t)( ( virt_addr & 0x00000000001FF000UL ) >> 12 );

    // Acquire lock
    axk_spinlock_acquire( &g_lock );

    uint64_t* pml4_array    = (uint64_t*) g_ptr_kpml4;
    uint64_t* pdpt_array    = NULL;
    uint64_t* pdt_array     = NULL;
    uint64_t* pt_array      = NULL;

    // Either get existing PDPT or create a new one
    if( !AXK_CHECK_FLAG( pml4_array[ pml4_index ], AXK_FLAG_PAGEMAP_PRESENT ) )
    {
        AXK_PAGE_ID new_page;
        if( !axk_page_acquire( 1UL, &new_page, AXK_PROCESS_KERNEL, AXK_FLAG_PAGE_CLEAR_ON_LOCK | AXK_FLAG_PAGE_TYPE_PTABLE ) )
        {
            axk_panic( "Memory Map: failed to allocate page needed for kernel memory map" );
        }

        pml4_array[ pml4_index ] = ( ( new_page * AXK_PAGE_SIZE ) | AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_WRITABLE );
        pdpt_array = (uint64_t*)( ( new_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );
    }
    else
    {
        pdpt_array = (uint64_t*)( ( pml4_array[ pml4_index ] & 0xFFFFFFFFFF000UL ) + AXK_KERNEL_VA_PHYSICAL );
    }

    // Either get existing PDT or create a new one
    if( !AXK_CHECK_FLAG( pdpt_array[ pdpt_index ], AXK_FLAG_PAGEMAP_PRESENT ) )
    {
        AXK_PAGE_ID new_page;
        if( !axk_page_acquire( 1UL, &new_page, AXK_PROCESS_KERNEL, AXK_FLAG_PAGE_CLEAR_ON_LOCK | AXK_FLAG_PAGE_TYPE_PTABLE ) )
        {
            axk_panic( "Memory Map: failed to allocate page needed for kernel memory map" );
        }

        pdpt_array[ pdpt_index ] = ( ( new_page * AXK_PAGE_SIZE ) | AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_WRITABLE );
        pdt_array = (uint64_t*)( ( new_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );
    }
    else
    {
        pdt_array = (uint64_t*)( ( pdpt_array[ pdpt_index ] & 0xFFFFFFFFFF000UL ) + AXK_KERNEL_VA_PHYSICAL );
    }

    // Either get existing PT or create new one, also, handle huge pages
    if( AXK_CHECK_FLAG( pdt_array[ pdt_index ], AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_HUGE_PAGE ) )
    {
        if( !b_overwrite ) { return false; }
    }
    else if( AXK_CHECK_FLAG( pdt_array[ pdt_index ], AXK_FLAG_PAGEMAP_PRESENT ) )
    {
        pt_array = (uint64_t*)( ( pdt_array[ pdt_index ] & 0xFFFFFFFFFF000UL ) + AXK_KERNEL_VA_PHYSICAL );
    }

    if( pt_array == NULL )
    {
        AXK_PAGE_ID new_page;
        if( !axk_page_acquire( 1UL, &new_page, AXK_PROCESS_KERNEL, AXK_FLAG_PAGE_CLEAR_ON_LOCK | AXK_FLAG_PAGE_TYPE_PTABLE ) )
        {
            axk_panic( "Memory Map: failed to allocate page needed for kernel memory map" );
        }

        pdt_array[ pdt_index ] = ( ( new_page * AXK_PAGE_SIZE ) | AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_WRITABLE );
        pt_array = (uint64_t*)( ( new_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );
    }

    // Write the page table entry
    pt_array[ pt_index ] = ( ( page * AXK_PAGE_SIZE ) | AXK_FLAG_PAGEMAP_PRESENT | AXK_FLAG_PAGEMAP_WRITABLE | flags );

    // Release the spinlock
    axk_spinlock_release( &g_lock );
    return true;
}


AXK_PAGE_ID axk_kunmap( uint64_t virt_addr )
{
    // Validate the parameters
    if( virt_addr < AXK_KERNEL_VA_HEAP || virt_addr >= AXK_KERNEL_VA_IMAGE || ( virt_addr % AXK_PAGE_SIZE ) != 0UL ) { return false; }

    // Calculate indicies
    uint32_t pml4_index  = (uint32_t)( ( virt_addr & 0x0000FF8000000000UL ) >> 39 );
    uint32_t pdpt_index  = (uint32_t)( ( virt_addr & 0x0000007FC0000000UL ) >> 30 );
    uint32_t pdt_index   = (uint32_t)( ( virt_addr & 0x000000003FE00000UL ) >> 21 );
    uint32_t pt_index    = (uint32_t)( ( virt_addr & 0x00000000001FF000UL ) >> 12 );

    uint64_t* pml4_array    = (uint64_t*) g_ptr_kpml4;
    uint64_t* pdpt_array    = NULL;
    uint64_t* pdt_array     = NULL;
    uint64_t* pt_array      = NULL;

    // Acquire lock
    axk_spinlock_acquire( &g_lock );

    if( !AXK_CHECK_ANY_FLAG( pml4_array[ pml4_index ], AXK_FLAG_PAGEMAP_PRESENT ) )
    {
        axk_spinlock_release( &g_lock );
        return 0UL;
    }

    pdpt_array = (uint64_t*)( ( pml4_array[ pml4_index ] & 0xFFFFFFFFFF000UL ) + AXK_KERNEL_VA_PHYSICAL );
    if( !AXK_CHECK_ANY_FLAG( pdpt_array[ pdpt_index ], AXK_FLAG_PAGEMAP_PRESENT ) )
    {
        axk_spinlock_release( &g_lock );
        return 0UL;
    }

    pdt_array = (uint64_t*)( ( pdpt_array[ pdpt_index ] & 0xFFFFFFFFFF000UL ) + AXK_KERNEL_VA_PHYSICAL );
    if( !AXK_CHECK_ANY_FLAG( pdt_array[ pdt_index ], AXK_FLAG_PAGEMAP_PRESENT ) )
    {
        axk_spinlock_release( &g_lock );
        return 0UL;
    }

    uint64_t out_page = 0UL;
    if( AXK_CHECK_ANY_FLAG( pdt_array[ pdt_index ], AXK_FLAG_PAGEMAP_HUGE_PAGE ) )
    {
        // Pointing to a huge page, but, not the start of the huge page
        if( pt_index != 0 ) { axk_spinlock_release( &g_lock ); return 0UL; }

        // Return the physical page this virtual page is mapped to
        out_page = ( pdt_array[ pdt_index ] & 0x000FFFFFFFE00000UL ) / AXK_PAGE_SIZE;
        pdt_array[ pdt_index ] = 0UL;
    }
    else
    {
        uint64_t pt_physaddr = ( pdt_array[ pdt_index ] & 0x000FFFFFFFFFF000UL );
        pt_array = (uint64_t*)( pt_physaddr + AXK_KERNEL_VA_PHYSICAL );
        pt_physaddr /= AXK_PAGE_SIZE;

        // Get the physical address pointed to by the mapping
        out_page = ( ( pt_array[ pt_index ] & 0x000FFFFFFFFFF000UL ) / AXK_PAGE_SIZE );
        pt_array[ pt_index ] = 0UL;

        // Check if there are any mappings left in this PT
        bool b_entries = false;
        for( uint32_t i = 0; i < 512; i++ )
        {
            if( AXK_CHECK_ANY_FLAG( pt_array[ i ], AXK_FLAG_PAGEMAP_PRESENT ) )
            {
                b_entries = true;
                break;
            }
        }

        // If there arent any entries, we can free it
        if( !b_entries )
        {
            if( !axk_page_release( 1UL, &pt_physaddr, true ) )
            {
                axk_panic( "Memory Map: failed to free unused page table" );
            }

            // Remove entry from PDT pointing to this PT
            pdt_array[ pdt_index ] = 0UL;
        }
    }

    // Check if there are any remaining PDT entries
    bool b_pdt_entries = false;
    for( uint32_t i = 0; i < 512; i++ )
    {
        if( AXK_CHECK_ANY_FLAG( pdt_array[ i ], AXK_FLAG_PAGEMAP_PRESENT ) )
        {
            b_pdt_entries = true;
            break;
        }
    }

    // If not, free the page it is stored on and remove the PDPT entry pointing to it
    if( !b_pdt_entries )
    {
        uint64_t pdt_pageid = ( ( (uint64_t)pdt_array ) - AXK_KERNEL_VA_PHYSICAL ) / AXK_PAGE_SIZE;
        if( !axk_page_release( 1UL, &pdt_pageid, true ) )
        {
            axk_panic( "Memory Map: failed to free unused page table" );
        }

        pdpt_array[ pdpt_index ] = 0UL;
    }

    // Check if there are any entries left in the PDPT
    bool b_pdpt_entries = false;
    for( uint32_t i = 0; i < 512; i++ )
    {
        if( AXK_CHECK_ANY_FLAG( pdpt_array[ i ], AXK_FLAG_PAGEMAP_PRESENT ) )
        {
            b_pdpt_entries = true;
            break;
        }
    }

    // Like before, free the page its on, remove pml4 entry
    if( !b_pdpt_entries )
    {
        uint64_t pdpt_pageid = ( ( (uint64_t) pdpt_array ) - AXK_KERNEL_VA_PHYSICAL ) / AXK_PAGE_SIZE;
        if( !axk_page_release( 1UL, &pdpt_pageid, true ) )
        {
            axk_panic( "Memory Map: failed to free unused page table" );
        }

        pml4_array[ pml4_index ] = 0UL;
    }

    axk_spinlock_release( &g_lock );
    return out_page;
}


uint64_t axk_kcheckmap( uint64_t virt_addr )
{
    // TODO: We can add optimizations once we know everything works as expected
    if( virt_addr < AXK_PAGE_SIZE ) { return virt_addr; }
    
    // Calculate indicies
    uint32_t pml4_index  = (uint32_t)( ( virt_addr & 0x0000FF8000000000UL ) >> 39 );
    uint32_t pdpt_index  = (uint32_t)( ( virt_addr & 0x0000007FC0000000UL ) >> 30 );
    uint32_t pdt_index   = (uint32_t)( ( virt_addr & 0x000000003FE00000UL ) >> 21 );
    uint32_t pt_index    = (uint32_t)( ( virt_addr & 0x00000000001FF000UL ) >> 12 );
    uint32_t offset      = (uint32_t)( ( virt_addr & 0x0000000000000FFFUL ) );

    uint64_t* pml4_array    = (uint64_t*) g_ptr_kpml4;
    uint64_t* pdpt_array    = NULL;
    uint64_t* pdt_array     = NULL;
    uint64_t* pt_array      = NULL;

    // Acquire the lock
    axk_spinlock_acquire( &g_lock );

    if( !AXK_CHECK_ANY_FLAG( pml4_array[ pml4_index ], AXK_FLAG_PAGEMAP_PRESENT ) )
    {
        axk_spinlock_release( &g_lock );
        return 0UL;
    }

    pdpt_array = (uint64_t*)( ( pml4_array[ pml4_index ] & 0xFFFFFFFFFF000UL ) + AXK_KERNEL_VA_PHYSICAL );
    if( !AXK_CHECK_ANY_FLAG( pdpt_array[ pdpt_index ], AXK_FLAG_PAGEMAP_PRESENT ) )
    {
        axk_spinlock_release( &g_lock );
        return 0UL;
    }

    pdt_array = (uint64_t*)( ( pdpt_array[ pdpt_index ] & 0xFFFFFFFFFF000UL ) + AXK_KERNEL_VA_PHYSICAL );
    if( !AXK_CHECK_ANY_FLAG( pdt_array[ pdt_index ], AXK_FLAG_PAGEMAP_PRESENT ) )
    {
        axk_spinlock_release( &g_lock );
        return 0UL;
    }

    if( AXK_CHECK_ANY_FLAG( pdt_array[ pdt_index ], AXK_FLAG_PAGEMAP_HUGE_PAGE ) )
    {
        // Extract the physical offset
        axk_spinlock_release( &g_lock );
        return( ( pdt_array[ pdt_index ] & 0x000FFFFFFFE00000UL ) + ( pt_index | offset ) );
    }
    else
    {
        pt_array = (uint64_t*)( ( pdt_array[ pdt_index ] & 0xFFFFFFFFFF000UL ) + AXK_KERNEL_VA_PHYSICAL );
        if( AXK_CHECK_ANY_FLAG( pt_array[ pt_index ], AXK_FLAG_PAGEMAP_PRESENT ) )
        {
            axk_spinlock_release( &g_lock );
            return( ( pt_array[ pt_index ] & 0xFFFFFFFFFF000UL ) + offset );
        }
        else
        {
            axk_spinlock_release( &g_lock );
            return 0UL;
        }
    }
}


uint64_t axk_acquire_shared_address( uint64_t page_count )
{
    // TODO: Improve this function to actually track pages that are being used in the kernel
    // Validate parameter
    if( page_count == 0UL || page_count > 0xFFFFFFFFUL ) { return 0UL; }

    return AXK_KERNEL_VA_SHARED + ( axk_atomic_fetch_add_uint64( &g_shared_counter, page_count, MEMORY_ORDER_SEQ_CST ) * 0x1000UL );
}


void axk_release_shared_address( uint64_t address, uint64_t page_count )
{
    // TODO
}

#endif
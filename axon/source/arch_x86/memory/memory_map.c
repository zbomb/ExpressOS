/*==============================================================
    Axon Kernel - Memory Map (x86)
    2021, Zachary Berry
    axon/source/arch_x86/memory/memory_map.c
==============================================================*/
#ifdef __x86_64__

#include "axon/kernel/kernel.h"
#include "axon/memory/memory_map.h"
#include "axon/memory/memory_private.h"
#include "axon/gfx/basic_terminal_private.h"
#include "axon/kernel/panic.h"
#include "axon/kernel/boot_params.h"
#include "axon/memory/page_allocator.h"


/*
    State
*/
static struct axk_memory_map_t g_kernel_map;
static bool g_init;

/*
    Externs
*/
extern char axk_pml4;

/*
    Private Constants
*/
#define PAGE_MAP_ENTRY_PRESENT          0b1
#define PAGE_MAP_ENTRY_WRITABLE         0b10
#define PAGE_MAP_ENTRY_KERNEL_ONLY      0b100
#define PAGE_MAP_ENTRY_WRITE_THROUGH    0b1000
#define PAGE_MAP_ENTRY_DISABLE_CACHE    0b10000
#define PAGE_MAP_ENTRY_ACCESSED         0b100000
#define PAGE_MAP_ENTRY_EXEC_DISABLE     ( 1UL << 63 )   // Only can use if IA32_EFER.NXE = 1

#define PAGE_MAP_MEM_ENTRY_DIRTY        0b1000000
#define PAGE_MAP_MEM_ENTRY_HUGE         0b10000000
#define PAGE_MAP_MEM_ENTRY_GLOBAL       0b100000000     // Only can use if CR4.PGE = 1

#define PAGE_MAP_ENTRY_4KB_MASK         0xFFFFFFFFFF000UL
#define PAGE_MAP_ENTRY_2MB_MASK         0xFFFFFFFE00000UL
#define PAGE_MAP_ENTRY_1GB_MASK         0xFFFFFC0000000UL


/*
    Function Implementations
*/
void axk_kmap_init( struct tzero_payload_parameters_t* in_params )
{
    // Guard against this being called twice
    if( g_init ) { return; }
    g_init = true;

    // Setup the kernel memory map, so we can modify it
    axk_spinlock_init( &( g_kernel_map.lock ) );

    g_kernel_map.process_id     = AXK_PROCESS_KERNEL;
    g_kernel_map.pml4           = (void*)( &axk_pml4 );
    uint64_t* pml4_table        = (uint64_t*)( g_kernel_map.pml4 );

    // Map all physical memory to the high kernel address space
    // We want to include MMIO mapped memory as well!
    uint64_t max_address = 0UL;
    for( uint32_t i = 0; i < in_params->memory_map.count; i++ )
    {
        struct tzero_memory_entry_t* entry = in_params->memory_map.list + i;
        uint64_t max = entry->base_address + ( entry->page_count * AXK_PAGE_SIZE );
        if( max > max_address ) { max_address = max; }
    }

    // Ensure we map enough to include the framebuffer as well
    uint64_t framebuffer_begin  = in_params->framebuffer.phys_addr;
    uint64_t framebuffer_end    = framebuffer_begin + in_params->framebuffer.size;
    if( max_address < framebuffer_end ) { max_address = framebuffer_end; }

    uint64_t max_huge_pages     = ( max_address + ( AXK_HUGE_PAGE_SIZE - 1UL ) ) / AXK_HUGE_PAGE_SIZE;
    uint64_t active_counter     = 0UL;

    for( uint64_t i = 0; i < max_huge_pages; i++ )
    {
        // Determine if there are any entries in the memory map within this range, so we dont map pages that arent needed
        uint64_t page_begin     = i * AXK_HUGE_PAGE_SIZE;
        uint64_t page_end       = page_begin + AXK_HUGE_PAGE_SIZE;

        bool b_active = false;
        for( uint32_t i = 0; i < in_params->memory_map.count; i++ )
        {
            uint64_t base_addr = in_params->memory_map.list[ i ].base_address;
            uint64_t end_addr = base_addr + ( in_params->memory_map.list[ i ].page_count * AXK_PAGE_SIZE );

            // Check if these ranges overlap at all
            if( ( base_addr >= page_begin && base_addr < page_end ) ||
                ( end_addr > page_begin && end_addr <= page_end ) ||
                ( base_addr <= page_begin && end_addr >= page_end ) )
            {
                b_active = true;
                break;
            }
            else if( base_addr > page_end )
            {
                // Since the map is sorted, we can exit once the page start is past the range were checking
                break;
            }
        }

        // Check if this page is part of the framebuffer
        if( ( framebuffer_begin >= page_begin && framebuffer_begin < page_end ) ||
            ( framebuffer_end > page_begin && framebuffer_end <= page_end ) ||
            ( framebuffer_begin <= page_begin && framebuffer_end >= page_end ) )
        {
            b_active = true;
        }

        // Skip pages not backed by memory or IO
        if( !b_active ) { continue; }
        active_counter++;

        // Now, lets calculate the virtual address were mapping the physical page to, and determine the index for each level of page table
        uint64_t virt_addr = ( i * AXK_HUGE_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL;
        uint32_t pml4_index  = (uint32_t)( ( virt_addr & 0x0000FF8000000000UL ) >> 39 );
        uint32_t pdpt_index  = (uint32_t)( ( virt_addr & 0x0000007FC0000000UL ) >> 30 );
        uint32_t pdt_index   = (uint32_t)( ( virt_addr & 0x000000003FE00000UL ) >> 21 );
        
        uint64_t* pdpt_table    = NULL;
        uint64_t* pdt_table     = NULL;

        // Check if we need to allocate a page for the PDPT, or update the flags
        if( !AXK_CHECK_FLAG( pml4_table[ pml4_index ], PAGE_MAP_ENTRY_PRESENT ) )
        {
            uint64_t page_id;
            if( !axk_page_acquire( 1UL, &page_id, AXK_PROCESS_KERNEL, AXK_PAGE_TYPE_PAGE_TABLE, AXK_PAGE_FLAG_NONE ) )
            {
                axk_panic( "Failed to allocate pages required to setup virtual memory mappings" );
            }

            pdpt_table = (uint64_t*)( page_id * AXK_PAGE_SIZE );
            memset( pdpt_table, 0, AXK_PAGE_SIZE );
            pml4_table[ pml4_index ] = ( ( page_id * AXK_PAGE_SIZE ) | PAGE_MAP_ENTRY_PRESENT | PAGE_MAP_ENTRY_WRITABLE );
        }
        else
        {
            AXK_SET_FLAG( pml4_table[ pml4_index ], PAGE_MAP_ENTRY_WRITABLE );
            pdpt_table = (uint64_t*)( pml4_table[ pml4_index ] & PAGE_MAP_ENTRY_4KB_MASK );
        }

        // Check the PDPT entry
        if( !AXK_CHECK_FLAG( pdpt_table[ pdpt_index ], PAGE_MAP_ENTRY_PRESENT ) )
        {
            uint64_t page_id;
            if( !axk_page_acquire( 1UL, &page_id, AXK_PROCESS_KERNEL, AXK_PAGE_TYPE_PAGE_TABLE, AXK_PAGE_FLAG_NONE ) )
            {
                axk_panic( "Failed to allocate pages required to setup virtual memory mappings" );
            }

            pdt_table = (uint64_t*)( page_id * AXK_PAGE_SIZE );
            memset( pdt_table, 0, AXK_PAGE_SIZE );
            pdpt_table[ pdpt_index ] = ( ( page_id * AXK_PAGE_SIZE ) | PAGE_MAP_ENTRY_PRESENT | PAGE_MAP_ENTRY_WRITABLE );
        }
        else
        {
            AXK_SET_FLAG( pdpt_table[ pdpt_index ], PAGE_MAP_ENTRY_WRITABLE );
            pdt_table = (uint64_t*)( pdpt_table[ pdpt_index ] & PAGE_MAP_ENTRY_4KB_MASK );
        }

        // Write the PDT entry
        pdt_table[ pdt_index ] = ( ( i * AXK_HUGE_PAGE_SIZE ) | PAGE_MAP_MEM_ENTRY_HUGE | PAGE_MAP_ENTRY_PRESENT | PAGE_MAP_ENTRY_WRITABLE );
    }

    // Update pointers in systems already initialized, because were going to remove the identity mapped UEFI mappings
    axk_basicterminal_update_pointers();
    axk_page_allocator_update_pointers();
    
    // And now we can get rid of the UEFI mappings, since the pages it uses arent managed by the page allocator system, we can simply clear the PML4 entries
    for( uint32_t i = 0; i < 256; i++ )
    {
        pml4_table[ i ] = 0x00UL;
    }

    axk_basicterminal_prints( "Memory Map: Initialized kernel memory map manager! Physical Memory Range: " );
    axk_basicterminal_printh64( AXK_KERNEL_VA_PHYSICAL, true );
    axk_basicterminal_prints( " to " );
    axk_basicterminal_printh64( AXK_KERNEL_VA_PHYSICAL + ( max_huge_pages * AXK_HUGE_PAGE_SIZE ), true );
    axk_basicterminal_prints( "\tActive Pages (2MB): " );
    axk_basicterminal_printu64( active_counter );
    axk_basicterminal_printnl();
}


bool axk_memory_map_create( struct axk_memory_map_t* in_map, uint32_t process_id )
{
    if( in_map == NULL || process_id == AXK_PROCESS_INVALID ) { return false; }

    // Setup the lock and process identifier
    axk_spinlock_init( &( in_map->lock ) );
    in_map->process_id = process_id;

    // Allocate a page for the PML4
    uint64_t pml4_page;
    if( !axk_page_acquire( 1UL, &pml4_page, process_id, AXK_PAGE_TYPE_PAGE_TABLE, AXK_PAGE_FLAG_CLEAR ) )
    {
        return false;
    }
    
    // Store address we can access PML4 through
    in_map->pml4 = (uint64_t*)( pml4_page * AXK_PAGE_SIZE );
    return true;
}


void axk_memory_map_destroy( struct axk_memory_map_t* in_map )
{
    if( in_map == NULL || in_map->pml4 == NULL ) { return; }

    // Ensure someone isnt trying to destroy the kernel memory map
    if( in_map == &g_kernel_map || in_map->pml4 == g_kernel_map.pml4 ) 
    {
        axk_panic( "Attempt to destroy the kernel memory map!" );
    }

    // Iterate through the page tables and release all of the physical pages used for each of them
    uint64_t* pml4 = (uint64_t*)( (uint64_t)( in_map->pml4 ) + AXK_KERNEL_VA_PHYSICAL );
    for( uint32_t pml4_index = 0; pml4_index < 512; pml4_index++ )
    {
        uint64_t pml4_entry = pml4[ pml4_index ];
        if( AXK_CHECK_FLAG( pml4_entry, PAGE_MAP_ENTRY_PRESENT ) )
        {
            for( uint32_t pdpt_index = 0; pdpt_index < 512; pdpt_index++ )
            {
                uint64_t pdpt_entry = ( (uint64_t*)( ( pml4_entry & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL ) )[ pdpt_index ];
                if( AXK_CHECK_FLAG( pdpt_entry, PAGE_MAP_ENTRY_PRESENT ) )
                {
                    for( uint32_t pdt_index = 0; pdt_index < 512; pdt_index++ )
                    {
                        uint64_t pdt_entry = ( (uint64_t*)( ( pdpt_entry & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL )[ pdt_index ];
                        if( AXK_CHECK_FLAG( pdt_entry, PAGE_MAP_ENTRY_PRESENT ) )
                        {
                            uint64_t pt_addr = ( pdt_entry & PAGE_MAP_ENTRY_4KB_MASK ) / AXK_PAGE_SIZE;
                            axk_page_release( 1UL, &pt_addr, AXK_PAGE_FLAG_NONE );
                        }
                    }
                }

                uint64_t pdt_addr = ( pdpt_entry & PAGE_MAP_ENTRY_4KB_MASK ) / AXK_PAGE_SIZE;
                axk_page_release( 1UL, &pdt_addr, AXK_PAGE_FLAG_NONE );
            }
        }

        uint64_t pdpt_addr = ( pml4_entry & PAGE_MAP_ENTRY_PRESENT ) / AXK_PAGE_SIZE;
        axk_page_release( 1UL, &pdpt_addr, AXK_PAGE_FLAG_NONE );
    }

    // Finally, release PML4
    uint64_t pml4_addr = (uint64_t)( in_map->pml4 ) / AXK_PAGE_SIZE;
    axk_page_release( 1UL, pml4_addr, AXK_PAGE_FLAG_NONE );

    in_map->pml4 = NULL;
}


void axk_memory_map_lock( struct axk_memory_map_t* in_map )
{
    if( in_map != NULL ) { axk_spinlock_acquire( &( in_map->lock ) ); }
}


void axk_memory_map_unlock( struct axk_memory_map_t* in_map )
{
    if( in_map != NULL ) { axk_spinlock_release( &( in_map->lock ) ); }
}


bool axk_memory_map_add( struct axk_memory_map_t* in_map, uint64_t in_vaddr, uint64_t in_page_id, uint64_t* out_page_id, uint32_t flags )
{
    if( in_map == NULL || in_map->pml4 == NULL || ( in_vaddr % AXK_PAGE_SIZE ) != 0UL ) { return false; }
    
    uint32_t pml4_index = (uint32_t)( ( in_vaddr & 0x0000FF8000000000UL ) >> 39 );
    uint32_t pdpt_index = (uint32_t)( ( in_vaddr & 0x0000007FC0000000UL ) >> 30 );
    uint32_t pdt_index  = (uint32_t)( ( in_vaddr & 0x000000003FE00000UL ) >> 21 );
    uint32_t pt_index   = (uint32_t)( ( in_vaddr & 0x00000000001FF000UL ) >> 12 );

    uint64_t* pml4      = (uint64_t*)( (uint64_t)( in_map->pml4 ) + AXK_KERNEL_VA_PHYSICAL );
    uint64_t* pdpt      = NULL;
    uint64_t* pdt       = NULL;
    uint64_t* pt        = NULL;

    uint64_t pdpt_page  = 0UL;
    uint64_t pdt_page   = 0UL;
    uint64_t pt_page    = 0UL;
    
    // Find or create PDPT
    if( !AXK_CHECK_ANY_FLAG( pml4[ pml4_index ], PAGE_MAP_ENTRY_PRESENT ) )
    {
        if( !axk_page_acquire( 1UL, &pdpt_page, in_map->process_id, AXK_PAGE_TYPE_PAGE_TABLE, AXK_PAGE_FLAG_CLEAR ) ) { return false; }
        pdpt = (uint64_t*)( ( pdpt_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );
        pml4[ pml4_index ] = ( PAGE_MAP_ENTRY_PRESENT | PAGE_MAP_ENTRY_WRITABLE | ( pdpt_page * AXK_PAGE_SIZE ) );
    }
    else
    {
        pdpt = (uint64_t*)( ( pml4[ pml4_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
    }

    // Find or create PDT
    if( !AXK_CHECK_ANY_FLAG( pdpt[ pdpt_index ], PAGE_MAP_ENTRY_PRESENT ) )
    {
        if( !axk_page_acquire( 1UL, &pdt_page, in_map->process_id, AXK_PAGE_TYPE_PAGE_TABLE, AXK_PAGE_FLAG_CLEAR ) )
        {
            if( pdpt_page != 0UL ) { axk_page_release( 1UL, &pdpt_page, AXK_PAGE_FLAG_NONE ); }
            return false;
        }

        pdt = (uint64_t*)( ( pdt_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );
        pdpt[ pdpt_index ] = ( PAGE_MAP_ENTRY_PRESENT | PAGE_MAP_ENTRY_WRITABLE | ( pdt_page * AXK_PAGE_SIZE ) );
    }
    else
    {
        pdt = (uint64_t*)( ( pdpt[ pdpt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
    }

    // Find or create PT
    if( !AXK_CHECK_ANY_FLAG( pdt[ pdt_index ], PAGE_MAP_ENTRY_PRESENT ) )
    {
        if( !axk_page_acquire( 1UL, &pt_page, in_map->process_id, AXK_PAGE_TYPE_PAGE_TABLE, AXK_PAGE_FLAG_CLEAR ) )
        {
            if( pdpt_page != 0UL ) { axk_page_release( 1UL, &pdpt_page, AXK_PAGE_FLAG_NONE ); }
            if( pdt_page != 0UL ) { axk_page_release( 1UL, &pdt_page, AXK_PAGE_FLAG_NONE ); }
            return false;
        }

        pt = (uint64_t*)( ( pt_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );
        pdt[ pdt_index ] = ( PAGE_MAP_ENTRY_PRESENT | PAGE_MAP_ENTRY_WRITABLE | ( pt_page * AXK_PAGE_SIZE ) );
    }
    else
    {
        pt = (uint64_t*)( ( pdt[ pdt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
    }

    // Check if we are overwriting an existing entry
    if( AXK_CHECK_FLAG( pt[ pt_index ], PAGE_MAP_ENTRY_PRESENT ) )
    {
        if( out_page_id == NULL )
        {
            // Overwrite is not allowed!
            if( pdpt_page != 0UL ) { axk_page_release( 1UL, &pdpt_page, AXK_PAGE_FLAG_NONE ); }
            if( pdt_page != 0UL ) { axk_page_release( 1UL, &pdt_page, AXK_PAGE_FLAG_NONE ); }
            if( pt_page != 0UL ) { axk_page_release( 1UL, &pt_page, AXK_PAGE_FLAG_NONE ); }

            return false;
        }

        *out_page_id = ( pt[ pt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) / AXK_PAGE_SIZE;
    }

    // Now, we can write the mapping in the page table, lets translate the flags to x86-specific versions of the flags
    uint64_t pt_entry = ( in_page_id * AXK_PAGE_SIZE ) | PAGE_MAP_ENTRY_PRESENT;

    if( !AXK_CHECK_FLAG( flags, AXK_MAP_FLAG_READ_ONLY ) )      { pt_entry |= PAGE_MAP_ENTRY_WRITABLE; }
    if( AXK_CHECK_FLAG( flags, AXK_MAP_FLAG_NO_EXEC ) )         { pt_entry |= PAGE_MAP_ENTRY_EXEC_DISABLE; }
    if( AXK_CHECK_FLAG( flags, AXK_MAP_FLAG_GLOBAL ) )          { pt_entry |= PAGE_MAP_MEM_ENTRY_GLOBAL; }
    if( AXK_CHECK_FLAG( flags, AXK_MAP_FLAG_NO_CACHE ) )        { pt_entry |= PAGE_MAP_ENTRY_DISABLE_CACHE; }
    if( AXK_CHECK_FLAG( flags, AXK_MAP_FLAG_KERNEL_ONLY ) )     { pt_entry |= PAGE_MAP_ENTRY_KERNEL_ONLY; }

    pt[ pt_index ] = pt_entry;
    return true;
}


bool axk_memory_map_remove( struct axk_memory_map_t* in_map, uint64_t in_vaddr, uint64_t* out_page_id )
{
    // Validate all of the parameters
    if( in_map == NULL || in_map->pml4 == NULL || ( in_vaddr % AXK_PAGE_SIZE ) != 0UL || out_page_id == NULL ) { return false; }

    // Get the index for each page table
    uint32_t pml4_index = (uint32_t)( ( in_vaddr & 0x0000FF8000000000UL ) >> 39 );
    uint32_t pdpt_index = (uint32_t)( ( in_vaddr & 0x0000007FC0000000UL ) >> 30 );
    uint32_t pdt_index  = (uint32_t)( ( in_vaddr & 0x000000003FE00000UL ) >> 21 );
    uint32_t pt_index   = (uint32_t)( ( in_vaddr & 0x00000000001FF000UL ) >> 12 );

    uint64_t* pml4      = (uint64_t*)( (uint64_t)( in_map->pml4 ) + AXK_KERNEL_VA_PHYSICAL );
    uint64_t* pdpt      = NULL;
    uint64_t* pdt       = NULL;
    uint64_t* pt        = NULL;

    // Traverse down to the lowest level page table (PT)
    if( AXK_CHECK_FLAG( pml4[ pml4_index ], PAGE_MAP_ENTRY_PRESENT ) )
    {
        pdpt = (uint64_t*)( ( pml4[ pml4_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
        if( AXK_CHECK_FLAG( pdpt[ pdpt_index ], PAGE_MAP_ENTRY_PRESENT ) )
        {
            pdt = (uint64_t*)( ( pdpt[ pdpt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
            if( AXK_CHECK_FLAG( pdt[ pdt_index ], PAGE_MAP_ENTRY_PRESENT ) )
            {
                pt = (uint64_t*)( ( pdt[ pdt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
            }
        }
    }

    // Check if we found the page table, and the entry is present
    if( pt == NULL || !AXK_CHECK_FLAG( pt[ pt_index ], PAGE_MAP_ENTRY_PRESENT ) ) { return false; }

    // Remove the entry, and pass the page identifier that was associated with this mapping back to the caller
    *out_page_id    = ( pt[ pt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) / AXK_PAGE_SIZE;
    pt[ pt_index ]  = 0x00UL;

    // We need to go through, and check if any of the page tables are empty and can be released
    // Loop through the PT, and if there are any remaining entries, then end the function
    for( uint32_t i = 0; i < 512; i++ )
    {
        if( AXK_CHECK_FLAG( pt[ i ], PAGE_MAP_ENTRY_PRESENT ) )
        {
            return true;
        }
    }

    // Free the PT, and clear the PDT entry
    uint64_t pt_page    = ( pdt[ pdt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) / AXK_PAGE_SIZE;
    pdt[ pdt_index ]    = 0UL;

    if( !axk_page_release_s( 1UL, &pt_page, in_map->process_id, AXK_PAGE_FLAG_NONE ) )
    {
        axk_basicterminal_prints( "[WARNING] Memory Map: Failed to release page used to store page table!\n" );
    }

    // Check for any remaining PDT entries
    for( uint32_t i = 0; i < 512; i++ )
    {
        if( AXK_CHECK_FLAG( pdt[ i ], PAGE_MAP_ENTRY_PRESENT ) )
        {
            return true;
        }
    }

    // Free the PDT, and clear the PDPT entry
    uint64_t pdt_page   = ( pdpt[ pdpt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) / AXK_PAGE_SIZE;
    pdpt[ pdpt_index ]  = 0UL;

    if( !axk_page_release_s( 1UL, &pdt_page, in_map->process_id, AXK_PAGE_FLAG_NONE ) )
    {
        axk_basicterminal_prints( "[WARNING] Memory Map: Failed to release page used to store page table!\n" );
    }

    // Check for any remaining PDPT entries
    for( uint32_t i = 0; i < 512; i++ )
    {
        if( AXK_CHECK_FLAG( pdpt[ i ], PAGE_MAP_ENTRY_PRESENT ) )
        {
            return true;
        }
    }

    // Free the PDPT, and clear the PML4 entry
    uint64_t pdpt_page  = ( pml4[ pml4_index ] & PAGE_MAP_ENTRY_4KB_MASK ) / AXK_PAGE_SIZE;
    pml4[ pml4_index ]  = 0UL;

    if( !axk_page_release_s( 1UL, &pdpt_page, in_map->process_id, AXK_PAGE_FLAG_NONE ) )
    {
        axk_basicterminal_prints( "[WARNING] Memory Map: Failed to release page used to store page table!\n" );
    }
    
    return true;
}


static inline uint64_t _parse_flags( uint64_t entry )
{
    uint64_t ret = 0UL;
    if( !AXK_CHECK_FLAG( entry, PAGE_MAP_ENTRY_WRITABLE ) )         { ret |= AXK_MAP_FLAG_READ_ONLY; }
    if( AXK_CHECK_FLAG( entry, PAGE_MAP_ENTRY_EXEC_DISABLE ) )      { ret |= AXK_MAP_FLAG_NO_EXEC; }
    if( AXK_CHECK_FLAG( entry, PAGE_MAP_MEM_ENTRY_GLOBAL ) )        { ret |= AXK_MAP_FLAG_GLOBAL; }
    if( AXK_CHECK_FLAG( entry, PAGE_MAP_ENTRY_DISABLE_CACHE ) )     { ret |= AXK_MAP_FLAG_NO_CACHE; }
    if( AXK_CHECK_FLAG( entry, PAGE_MAP_ENTRY_KERNEL_ONLY ) )       { ret |= AXK_MAP_FLAG_KERNEL_ONLY; }

    return ret;
}


bool axk_memory_map_translate( struct axk_memory_map_t* in_map, uint64_t in_addr, uint64_t* out_addr, uint32_t* out_flags )
{
    // Validate the parameters
    if( in_map == NULL || in_map->pml4 == NULL ) { return false; }

    uint32_t pml4_index = (uint32_t)( ( in_vaddr & 0x0000FF8000000000UL ) >> 39 );
    uint32_t pdpt_index = (uint32_t)( ( in_vaddr & 0x0000007FC0000000UL ) >> 30 );
    uint32_t pdt_index  = (uint32_t)( ( in_vaddr & 0x000000003FE00000UL ) >> 21 );
    uint32_t pt_index   = (uint32_t)( ( in_vaddr & 0x00000000001FF000UL ) >> 12 );

    uint64_t* pml4  = (uint64_t*)( (uint64_t)( in_map->pml4 ) + AXK_KERNEL_VA_PHYSICAL );
    if( AXK_CHECK_FLAG( pml4[ pml4_index ], PAGE_MAP_ENTRY_PRESENT ) )
    {
        uint64_t* pdpt = (uint64_t*)( ( pml4[ pml4_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
        if( AXK_CHECK_FLAG( pdpt[ pdpt_index ], PAGE_MAP_ENTRY_PRESENT ) )
        {
            uint64_t* pdt = (uint64_t*)( ( pdpt[ pdpt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
            if( AXK_CHECK_FLAG( pdt[ pdt_index ], PAGE_MAP_MEM_ENTRY_HUGE | PAGE_MAP_ENTRY_PRESENT ) )
            {
                if( out_addr != NULL ) { *out_addr = ( pdt[ pdt_index ] & PAGE_MAP_ENTRY_2MB_MASK ) + ( in_vaddr & 0x1FFFFFUL ); }
                if( out_flags != NULL ) { *out_flags = _parse_flags( pdt[ pdt_index ] ); }

                return true;
            }
            else if( AXK_CHECK_FLAG( pdt[ pdt_index ], PAGE_MAP_ENTRY_PRESENT ) )
            {
                uint64_t* pt = (uint64_t*)( ( pdt[ pdt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
                if( AXK_CHECK_FLAG( pt[ pt_index ], PAGE_MAP_ENTRY_PRESENT ) )
                {
                    if( out_addr != NULL ) { *out_addr = ( pt[ pt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + ( in_vaddr & 0xFFFUL ); }
                    if( out_flags != NULL ) { *out_flags = _parse_flags( pt[ pt_index ] ); }

                    return true;
                }
            }
        }
    }

    return false;
}


bool axk_memory_map_search( struct axk_memory_map_t* in_map, uint64_t in_page_id, uint64_t* out_virt_addr, uint32_t* out_flags )
{
    // Validate parameters
    if( in_map == NULL || in_map->pml4 == NULL ) { return false; }

    // So, we need to iterate through the entire map, looking for the desired page
    uint64_t* pml4 = (uint64_t*)( (uint64_t)( in_map->pml4 ) + AXK_KERNEL_VA_PHYSICAL );
    for( uint32_t i = 0; i < 512; i++ )
    {
        uint64_t pml4_entry = pml4[ i ];
        if( AXK_CHECK_FLAG( pml4_entry, PAGE_MAP_ENTRY_PRESENT ) )
        {
            uint64_t* pdpt = (uint64_t*)( (uint64_t)( pml4_entry & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
            for( uint32_t j = 0; j < 512; j++ )
            {
                uint64_t pdpt_entry = pdpt[ j ];
                if( AXK_CHECK_FLAG( pdpt_entry, PAGE_MAP_ENTRY_PRESENT ) )
                {
                    uint64_t* pdt = (uint64_t*)( (uint64_t)( pdpt_entry & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
                    for( uint32_t k = 0; k < 512; k++ )
                    {
                        uint64_t pdt_entry = pdt[ k ];

                        // For now, were going to ignore huge pages in this function, since it just complicates things and is only used in the kernel
                        if( AXK_CHECK_FLAG( pdt_entry, PAGE_MAP_ENTRY_PRESENT ) && !AXK_CHECK_FLAG( pdt_entry, PAGE_MAP_MEM_ENTRY_HUGE ) )
                        {
                            uint64_t* pt = (uint64_t*)( (uint64_t)( pdt_entry & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
                            for( uint32_t l = 0; l < 512; l++ )
                            {
                                uint64_t pt_entry = pt[ l ];
                                if( AXK_CHECK_FLAG( pt_entry, PAGE_MAP_ENTRY_PRESENT ) )
                                {
                                    // Check if this page is the one were looking for
                                    uint64_t entry_id = ( pt_entry & PAGE_MAP_ENTRY_4KB_MASK ) / AXK_PAGE_SIZE;
                                    if( entry_id == in_page_id )
                                    {
                                        if( out_virt_addr != NULL )
                                        {
                                            *out_virt_addr = ( (uint64_t)( i ) << 39 ) | ( (uint64_t)( j ) << 30 ) | ( (uint64_t)( k ) << 21 ) | ( (uint64_t)( l ) << 12 );
                                        }

                                        if( out_flags != NULL )
                                        {
                                            *out_flags = _parse_flags( pt_entry );
                                        }

                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}


bool axk_memory_map_copy( struct axk_memory_map_t* src_map, struct axk_memory_map_t* dst_map, uint64_t src_vaddr, uint64_t dst_vaddr )
{
    // Validate the parameters
    if( src_map == NULL || src_map->pml4 == NULL || dst_map == NULL || dst_map->pml4 == NULL || 
        ( src_vaddr % AXK_PAGE_SIZE ) != 0UL || ( dst_vaddr % AXK_PAGE_SIZE ) != 0UL ) { return false; }

    // To copy the mapping, we need to first read it, and then write it into the destination map, but we will fail if theres an existing entry there
    // Lets mask out each index we need
    uint32_t src_pml4_index = (uint32_t)( ( src_vaddr & 0x0000FF8000000000UL ) >> 39 );
    uint32_t src_pdpt_index = (uint32_t)( ( src_vaddr & 0x0000007FC0000000UL ) >> 30 );
    uint32_t src_pdt_index  = (uint32_t)( ( src_vaddr & 0x000000003FE00000UL ) >> 21 );
    uint32_t src_pt_index   = (uint32_t)( ( src_vaddr & 0x00000000001FF000UL ) >> 12 );
    uint32_t dst_pml4_index = (uint32_t)( ( dst_vaddr & 0x0000FF8000000000UL ) >> 39 );
    uint32_t dst_pdpt_index = (uint32_t)( ( dst_vaddr & 0x0000007FC0000000UL ) >> 30 );
    uint32_t dst_pdt_index  = (uint32_t)( ( dst_vaddr & 0x000000003FE00000UL ) >> 21 );
    uint32_t dst_pt_index   = (uint32_t)( ( dst_vaddr & 0x00000000001FF000UL ) >> 12 );

    uint64_t* src_pml4  = (uint64_t*)( (uint64_t)( src_map->pml4 ) + AXK_KERNEL_VA_PHYSICAL );
    uint64_t* src_pdpt  = NULL;
    uint64_t* src_pdt   = NULL;
    uint64_t* src_pt    = NULL;
    uint64_t* dst_pml4  = (uint64_t*)( (uint64_t)( dst_map->pml4 ) + AXK_KERNEL_VA_PHYSICAL );
    uint64_t* dst_pdpt  = NULL;
    uint64_t* dst_pdt   = NULL;
    uint64_t* dst_pt    = NULL;

    // Quickly lookup the source mapping
    if( !AXK_CHECK_FLAG( src_pml4[ src_pml4_index ] ) ) { return false; }
    src_pdpt = (uint64_t*)( ( src_pml4[ src_pml4_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
    if( !AXK_CHECK_FLAG( src_pdpt[ src_pdpt_index ], PAGE_MAP_ENTRY_PRESENT ) ) { return false; }
    src_pdt = (uint64_t*)( ( src_pdpt[ src_pdpt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
    if( !AXK_CHECK_FLAG( src_pdt[ src_pdt_index ], PAGE_MAP_ENTRY_4KB_MASK ) ) { return false; }
    src_pt = (uint64_t*)( ( src_pdt[ src_pdt_index ] & PAGE_MAP_ENTRY_4KB_MASK ) + AXK_KERNEL_VA_PHYSICAL );
    if( !AXK_CHECK_FLAG( src_pt[ src_pt_index ], PAGE_MAP_ENTRY_PRESENT ) ) { return false; }
    
    // Now, we need to insert into the destination map
    uint64_t dst_pdpt_page  = 0UL;
    uint64_t dst_pdt_page   = 0UL;

    if( !AXK_CHECK_FLAG( dst_pml4[ dst_pml4_index ], PAGE_MAP_ENTRY_PRESENT ) )
    {
        if( !axk_page_acquire( 1UL, &dst_pdpt_page, dst_map->process_id, AXK_PAGE_TYPE_PAGE_TABLE, AXK_PAGE_FLAG_CLEAR ) ) { return false; }
        dst_pdpt = (uint64_t*)( ( dst_pdpt_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );
        dst_pml4[ dst_pml4_index ] = ( dst_pdpt_page * AXK_PAGE_SIZE ) | PAGE_MAP_ENTRY_PRESENT | PAGE_MAP_ENTRY_WRITABLE;
    }

    if( !AXK_CHECK_FLAG( dst_pdpt[ dst_pdpt_index ], PAGE_MAP_ENTRY_PRESENT ) )
    {
        if( !axk_page_acquire( 1UL, &dst_pdt_page, dst_map->process_id, AXK_PAGE_TYPE_PAGE_TABLE, AXK_PAGE_FLAG_CLEAR ) ) 
        {
            if( dst_pdpt_page != 0UL ) { axk_page_release( 1UL, &dst_pdpt_page, AXK_PAGE_FLAG_NONE ); }
            return false;
        }

        dst_pdt = (uint64_t*)( ( dst_pdt_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );
        dst_pdpt[ dst_pdpt_index ] = ( dst_pdt_page * AXK_PAGE_SIZE ) | PAGE_MAP_ENTRY_PRESENT | PAGE_MAP_ENTRY_WRITABLE;
    }

    if( !AXK_CHECK_FLAG( dst_pdt[ dst_pdt_index ], PAGE_MAP_ENTRY_PRESENT ) )
    {
        uint64_t dst_pt_page;
        if( !axk_page_acquire( 1UL, &dst_pt_page, dst_map->process_id, AXK_PAGE_TYPE_PAGE_TABLE, AXK_PAGE_FLAG_CLEAR ) )
        {
            if( dst_pdpt_page != 0UL ) { axk_page_release( 1UL, &dst_pdpt_page, AXK_PAGE_FLAG_NONE ); }
            if( dst_pdt_page != 0UL ) { axk_page_release( 1UL, &dst_pdt_page, AXK_PAGE_FLAG_NONE ); }
            return false;
        }

        dst_pt = (uint64_t*)( ( dst_pt_page * AXK_PAGE_SIZE ) + AXK_KERNEL_VA_PHYSICAL );
        dst_pdt[ dst_pdt_index ] = ( dst_pt_page * AXK_PAGE_SIZE ) | PAGE_MAP_ENTRY_PRESENT | PAGE_MAP_ENTRY_WRITABLE;
    }

    // Check if there already is an entry in the destination map
    if( AXK_CHECK_FLAG( dst_pt[ dst_pt_index ], PAGE_MAP_ENTRY_PRESENT ) )
    {
        return false;
    }

    dst_pt[ dst_pt_index ] = src_pt[ src_pt_index ];
    return true;
}  


bool axk_memory_map_copy_range( struct axk_memory_map_t* src_map, struct axk_memory_map_t* dst_map, uint64_t start_src_vaddr, uint64_t end_src_vaddr, uint64_t dst_vaddr )
{
    // Validate the parameters
    if( src_map == NULL || src_map->pml4 == NULL || dst_map == NULL || dst_map->pml4 == NULL || ( end_src_vaddr % AXK_PAGE_SIZE ) != 0UL ||
        ( start_src_vaddr % AXK_PAGE_SIZE ) != 0UL || ( dst_vaddr % AXK_PAGE_SIZE ) != 0UL || start_src_vaddr >= end_src_vaddr ) { return false; }
    
    // Determine the number of pages were going to copy
    uint64_t page_count = ( end_src_vaddr - start_src_vaddr ) / AXK_PAGE_SIZE;

    // In theory, we could use dynamic memory to accomplish this, since the only other option is 
}


#endif
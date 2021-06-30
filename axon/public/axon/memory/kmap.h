/*==============================================================
    Axon Kernel - Kernel Page Mapping
    2021, Zachary Berry
    axon/public/axon/memory/kmap.h

    * Definitions located in 'axon/source/memory/map_mgr.c'
==============================================================*/

#pragma once
#include "axon/config.h"


/*
    axk_kmap
    * Adds a page mapping to the kernel address space
*/
bool axk_kmap( AXK_PAGE_ID page, uint64_t virt_addr, AXK_MAP_FLAGS flags );

/*
    axk_kunmap
    * Removes mappings from the kernel address space
    * Can be used to unmap one or multiple pages if desired
    * Returns true if all pages were unmapped, if it fails, NO mappings will be removed!
    
    Parameters:
        uint64_t count      => The number of pages to unmap, starting at the virtual address given
        uint64_t start_va   => The starting virtual address (must be multiple of AXK_PAGE_SIZE)
*/
AXK_PAGE_ID axk_kunmap( uint64_t virt_addr );

/*
    axk_kcheckmap
    * Checks if a virtual address is backed with a physical page
    * Returns the underlying physical address
    * Will return '0UL' if the address is not backed with physical memory currently
    
    Parameters:
        uint64_t virt_addr  => The virtual address to perform the lookup on
*/
uint64_t axk_kcheckmap( uint64_t virt_addr );

/*
    axk_acquire_shared_address
    * Acquires some shared address space without physical backing
    * This space can be used for MMIO, or files, etc..
*/
uint64_t axk_acquire_shared_address( uint64_t page_count );

/*
    axk_release_shared_address
    * Releases some shared address space so other systems can use it
*/
void axk_release_shared_address( uint64_t address, uint64_t page_count );

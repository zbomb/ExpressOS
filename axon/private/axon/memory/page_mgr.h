/*==============================================================
    Axon Kernel - Page Manager
    2021, Zachary Berry
    axon/public/axon/memory/page_mgr.h
    
    * Definitions located in 'axon/source/memory/page_alloc.c'
==============================================================*/

#pragma once
#include "axon/memory/page_alloc.h"


/*
    axk_page_mgr_init
    * Internal Function
    * Called to initialize the physical page allocator system
*/
bool axk_pagemgr_init( void );

/*
    axk_pagemgr_get_count
    * Internal Function
    * Gets the total number of 4KB pages available to the system
*/
uint64_t axk_pagemgr_get_count( void );

/*
    axk_pagemgr_get_physmem
    * Internal Function
    * Gets the total amount of physical bytes available to the system in bytes
*/
uint64_t axk_pagemgr_get_physmem( void );

/*
    axk_pagemgr_reclaim
    * Internal Function
    * Reclaims the memory used by boot systems, like ACPI, initrd, AP init, etc..
*/
void axk_pagemgr_reclaim( void );
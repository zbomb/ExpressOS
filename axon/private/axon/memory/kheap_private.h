/*==============================================================
    Axon Kernel - Kernel Heap (Private Functions)
    2021, Zachary Berry
    axon/public/axon/memory/kheap_private.h
    
    * Definitions located in 'axon/source/memory/kheap.c
==============================================================*/

#pragma once

#include "axon/config.h"


/*
    axk_kheap_init
    * Initializes the kernel heap, called from the entry point function
*/
bool axk_kheap_init( void );

/*
    axk_kheap_page_count
    * Gets the number of pages allocated to the kernel heap
*/
uint64_t axk_kheap_page_count( void );

/*
    axk_kheap_debug
    * Prints some debug info about the heap, including the current layout
*/
void axk_kheap_debug( void );
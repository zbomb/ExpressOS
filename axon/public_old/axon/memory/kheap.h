/*==============================================================
    Axon Kernel - Kernel Page Mapping
    2021, Zachary Berry
    axon/public/axon/memory/kheap.h
==============================================================*/

#pragma once

#include "axon/config.h"


/*
    NOTES:

    * In userspace, we might want to allocate memory space, without actually backing it with physical memory until the program attempts to
      write to the block of memory, at which point, we can page fault, and respond by applying some backing meemory

    * With the heap though, we have fancy inline tags to let us know the layout of the heap, meaning each page will be written to immediatley, unless
      the size of the heap entry is very large, and theres whole pages in between these tags. 

    * In kernel-space though, we want to have physical backing for the whole heap right now, this might change in the future, but right now, its 
      really the best solution to get us going. 
*/


/*
    axk_kheap_alloc
    * Allocates some space on the heap, at least 'sz' bytes large
    * Optionally, you can specify if you want zeroed memory to be returned
    * Returns virtual address that can be directly accessed
*/
void* axk_kheap_alloc( size_t sz, bool b_clear );

/*
    axk_kheap_realloc
    * Attempts to resize the heap entry, but if that fails, instead we create a new entry
    * This means, you MUST update the pointer with the returned pointer, since it might change
    * Optionally asks for zeroed memory to be returned
*/
void* axk_kheap_realloc( void* ptr, size_t new_sz, bool b_clear );

/*
    axk_kheap_free
    * Frees an allocated block of heap memory
*/
void axk_kheap_free( void* ptr );
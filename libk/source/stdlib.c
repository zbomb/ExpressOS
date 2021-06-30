/*==============================================================
    Axon Kernel Libk - stdlib.c
    2021, Zachary Berry
    libk/source/stdlib.c
==============================================================*/

#include "stdlib.h"
#include "axon/memory/kheap.h"


/*
    Function Implementaitons
*/

void* malloc( size_t size )
{
    if( size == 0UL ) { return NULL; }
    return axk_kheap_alloc( size, false );
}


void* calloc( size_t num, size_t size )
{
    if( num == 0UL || size == 0UL ) { return NULL; }
    return axk_kheap_alloc( num * size, true );
}


void* realloc( void* ptr, size_t new_size )
{
    if( new_size == 0UL )
    {
        axk_kheap_free( ptr );
        return NULL;
    }

    if( ptr == NULL )
    {
        return axk_kheap_alloc( new_size, false );
    }
    else
    {
        return axk_kheap_realloc( ptr, new_size, false );
    }
}


void free( void* ptr )
{
    axk_kheap_free( ptr );
}



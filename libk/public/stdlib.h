/*==============================================================
    Axon Kernel Libk - stdlib.h
    2021, Zachary Berry
    libk/public/string.h
==============================================================*/

#pragma once
#include <stdint.h>
#include <stddef.h>

/*
    Macros
*/
#define AXK_MALLOC_TYPE( _TY_ ) ( _TY_ *) malloc( sizeof( _TY_ ) )
#define AXK_CALLOC_TYPE( _TY_, _CNT_ ) ( _TY_ *) calloc( _CNT_, sizeof( _TY_ ) )
#define AXK_READ_UINT8( _POS_ ) *( (uint8_t*)( _POS_ ) )
#define AXK_READ_UINT16( _POS_ ) *( (uint16_t*)( _POS_ ) )
#define AXK_READ_UINT32( _POS_ ) *( (uint32_t*)( _POS_ ) )
#define AXK_READ_UINT64( _POS_ ) *( (uint64_t*)( _POS_ ) )
#define AXK_READ_INT8( _POS_ ) *( (int8_t*)( _POS_ ) )
#define AXK_READ_INT16( _POS_ ) *( (int16_t*)( _POS_ ) )
#define AXK_READ_INT32( _POS_ ) *( (int32_t*)( _POS_ ) )
#define AXK_READ_INT64( _POS_ ) *( (int64_t*)( _POS_ ) )


/*
    malloc
    * Public Function
    * Allocates memory, minimum size given as a parameter
    * Returns pointer to the allocated block of memory
*/
void* malloc( size_t size );

/*
    calloc
    * Public Function
    * Allocates memory for an array, with a given dimension and element size
    * The memory returned will be zero initialized
    * Returns a pointer to the memory block
*/
void* calloc( size_t num, size_t size );

/*
    realloc
    * Public Function
    * Changes the size of an allocated block of memory
    * Might change the address of the memory, so update the original pointer with the returned pointer
*/
void* realloc( void* ptr, size_t new_size );

/*
    free
    * Public Function
    * Deallocates a block of memory, previously allocated by malloc, calloc or realloc
*/
void free( void* ptr );




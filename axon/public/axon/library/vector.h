/*==============================================================
    Axon Kernel - Vector Container
    2021, Zachary Berry
    axon/public/axon/library/vector.h
==============================================================*/

#pragma once
#include "axon/config.h"


/*
    axk_vector_t (STRUCTURE)
    * Vector instance
    * Be sure to initialize with 'axk_vector_create'
    * Also, destroy using 'axk_vector_destroy'
*/
struct axk_vector_t
{
    void* buffer;
    uint64_t elem_count;
    uint64_t elem_capacity;
    uint64_t elem_size;
    uint8_t growth_factor;
};

/*
    axk_vector_create
    * Initializes a vector handle 
    * Must specify the element size 
*/
void axk_vector_create( struct axk_vector_t* in_handle, uint64_t elem_size );

/*
    axk_vector_create_with_capacity
    * Initializes a vector with a specified starting capacity
*/
void axk_vector_create_with_capacity( struct axk_vector_t* in_handle, uint64_t elem_size, uint64_t in_capacity );

/*
    axk_vector_destroy
    * Destroys a vector contained within a vector handle
*/
void axk_vector_destroy( struct axk_vector_t* in_handle );

/*
    axk_vector_copy
    * Copies one vector into another, uninitialized vector handle
*/
void axk_vector_copy( struct axk_vector_t* source, struct axk_vector_t* dest );

/*
    axk_vector_clear
    * Empties the contents of a vector
*/
void axk_vector_clear( struct axk_vector_t* in_handle );

/*
    axk_vector_index
    * Gets a pointer to the specified vector index
*/
void* axk_vector_index( struct axk_vector_t* in_handle, uint64_t index );

/*
    axk_vector_count
    * Gets the number of active elements in a vector
*/
uint64_t axk_vector_count( struct axk_vector_t* in_handle );

/*
    axk_vector_capacity
    * Gets the total capacity (in element count) of a vector
*/
uint64_t axk_vector_capacity( struct axk_vector_t* in_handle );

/*
    axk_vector_buffer
    * Gets the underlying storage buffer used by this vector
*/
void* axk_vector_buffer( struct axk_vector_t* in_handle );

/*
    axk_vector_insert
    * Insert a value into the vector at the specified index
    * Will fail if the index is past the end of the vector (plus 1)
*/
bool axk_vector_insert( struct axk_vector_t* in_handle, uint64_t index, void* in_elem );

/*
    axk_vector_insert_range
    * Inserts multiple elements from a buffer into this vector
    * Will fail if the target index is past the end of the vector (plus 1)
*/
bool axk_vector_insert_range( struct axk_vector_t* in_handle, uint64_t index, void* in_elem, uint64_t elem_count );

/*
    axk_vector_erase
    * Erases an entry in the vector, shifting remaining entries to fill the empty space
*/
bool axk_vector_erase( struct axk_vector_t* in_handle, uint64_t index );

/*
    axk_vector_erase_range
    * Erases multiple consecutive entries in the vector, shifting remaining entries to fill the empty space
*/
bool axk_vector_erase_range( struct axk_vector_t* in_handle, uint64_t index, uint64_t count );

/*
    axk_vector_resize
    * Changes the size of the vector, either growing or shrinking the number of active elements
*/
void axk_vector_resize( struct axk_vector_t* in_handle, uint64_t new_size );

/*
    axk_vector_push_back
    * Adds an element to the back of the vector
*/
void axk_vector_push_back( struct axk_vector_t* in_handle, void* in_elem );

/*
    axk_vector_push_front
    * Adds an element to the front of the vector, shifting all entires up one position
*/
void axk_vector_push_front( struct axk_vector_t* in_handle, void* in_elem );

/*
    axk_vector_pop_back
    * Removes an element from the back of the vector
*/
void axk_vector_pop_back( struct axk_vector_t* in_handle );

/*
    axk_vector_pop_front
    * Removes an element from the front of the vector
*/
void axk_vector_pop_front( struct axk_vector_t* in_handle );

/*
    axk_vector_get_back
    * Gets the element at the back of the vector
*/
void* axk_vector_get_back( struct axk_vector_t* in_handle );

/*
    axk_vector_get_front
    * Gets the element at the front of the vector
*/
void* axk_vector_get_front( struct axk_vector_t* in_handle );

/*
    axk_vector_compare
    * Comapres the raw data between two vector types
    * For some types, its probably better to check each element manually
    * Returns '0' if the two vectors are equal
*/
bool axk_vector_compare( struct axk_vector_t* lhs, struct axk_vector_t* rhs );



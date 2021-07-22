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
    void( *fn_finalize )( void* );
    void( *fn_copy )( void*, void* );
};

/*
    axk_vector_create
    * Creates a new vector, assigning the handle to the newly created vector, and automatically calculates the starting capacity
    * If the specified handle already refers to an existing vector, it will be destroyed and the handle will point to the newly created vector
    * You MUST provide an accurate element size, this indicates how much room to allocate per element in the underlying memory buffer
    * If the type you are inserting into the vector requires special handling on destruction, pass a pointer to the 'destructor function'
      into this call through 'finalize_func'. It will be automatically invoked when the element is removed or the vector is destroyed
    * If you do not pass a finalize function, the vector will only free the memory taken up by the element, and not perform special handling
    * This function will ALWAYS create a new vector and set 'in_handle' to refer to it, calling with 'in_handle' as NULL will cause a panic (or invalid elem size)
    * 'copy_func' allows a function to be provided, which will be invoked whenever an element needs to be copied INTO the vector, this can be due to
       calls to 'axk_vector_copy', 'axk_vector_insert[_range]', 'axk_vector_push_(front/back)'
    * If 'copy_func' is NULL, then copying will be performed by 'memcpy' automatically, copying "#elem_size" bytes from the source pointer to the internal storage
*/
void axk_vector_create( struct axk_vector_t* in_handle, uint64_t elem_size, void( *copy_func )( void*, void* ), void( *finalize_func )( void* ) );

/*
    axk_vector_create_with_capacity
    * Initializes a vector with a specified starting capacity
    * The only difference between 'axk_vector_create' is that you can specify the capacity the vector will start at
    * For more information see 'axk_vector_create'
*/
void axk_vector_create_with_capacity( struct axk_vector_t* in_handle, uint64_t elem_size, uint64_t in_capacity, void( *copy_func )( void*, void* ), void( *finalize_func )( void* ) );

/*
    axk_vector_destroy
    * Destroys a vector that the specified handle points to
    * If the handle doesnt point to a vector, then the call will simply return 
    * If a 'finalize function' was provided when the vector was created, it will be called on each element before the memory that element occupies
      is freed from the heap, allowing more complex structures to be stored within the vector
    * After this is called on a handle, it can be reused to create another vector with different parameters if desired
*/
void axk_vector_destroy( struct axk_vector_t* in_handle );

/*
    axk_vector_copy
    * Copies the vector that 'source' referes to, into a new vector, and sets the 'dest' handle to point to it
    * If 'dest' already contains a vector, it will be destroyed before being reassigned to the vector copy
    * If 'source' doesnt refer to a vector, then the vector refered to by 'dest' WILL STILL BE DESTROYED
    * 'fn_copy' is the function used to copy over each element indivisually into the new vector
    * If 'fn_copy' is NULL, a simple memory copy with occur to copy the buffer from 'source' to 'dest'
*/
void axk_vector_copy( struct axk_vector_t* dest, struct axk_vector_t* source );

/*
    axk_vector_move
    * Moves vector data from 'source' to 'dest'
    * After the call.. 'source' will be a NULL handle, and doesnt require deletion
    * After the call.. 'dest' will have whatever contents 'source' had before the call
    * Any existing vector referred to by 'dest' before the call will be destroyed before the move
    * This will NOT invoke the copy function provided when the vector was created! The internal buffer will simply be assigned to the destination vector
*/
void axk_vector_move( struct axk_vector_t* dest, struct axk_vector_t* source );

/*
    axk_vector_clear
    * Empties the contents of the vector refered to by 'in_handle'
    * The underlying vector isnt actually destroyed, it will still exist with the same parameters setup on creation
    * If 'in_handle' doesnt refer to a valid vector, the call will simply return
*/
void axk_vector_clear( struct axk_vector_t* in_handle );

/*
    axk_vector_index
    * Gets a pointer to an element stored within a vector, at the specified index
    * If 'in_handle' doesnt refer to a valid vector, NULL will be returned
    * If the specified index is outside of the vector bounds NULL will also be returned
*/
void* axk_vector_index( struct axk_vector_t* in_handle, uint64_t index );

/*
    axk_vector_count
    * Gets the number of active elements in the vector refered to by 'in_handle'
    * If 'in_handle' doesnt refer to a valid vector, 0UL will be returned
*/
uint64_t axk_vector_count( struct axk_vector_t* in_handle );

/*
    axk_vector_capacity
    * Gets the total capacity (in element count) of the vector refered to by 'in_handle'
    * This is NOT the total indexable range of the vector, since you can only index active elements
    * If 'in_handle' doesnt refer to a valid vector 0UL will be returned
*/
uint64_t axk_vector_capacity( struct axk_vector_t* in_handle );

/*
    axk_vector_buffer
    * Gets the underlying storage buffer used by this vector refered to by 'in_handle'
    * If 'in_handle' doesnt refer to a valid vector, NULL will be returned
    * All valid vectors contain at least one element of storage, so this will always return a valid pointer in that case
*/
void* axk_vector_buffer( struct axk_vector_t* in_handle );

/*
    axk_vector_insert
    * Insert a value into the vector refered to by 'in_handle', at the specified index
    * Valid inputs for 'index' are [0 to axk_vector_count( in_handle )] 
    * If 'index' == axk_vector_count( in_handle ), the element will be inserted at the end of the vector
    * If 'index' is out of the valid range, then NULL will be returned
    * 'in_elem' is a pointer to the data to be inserted, where the data pointed to by 'in_elem' NEEDS to be at least the size specified when creating the vector
    * If 'in_elem' is NULL, the function will fail and return NULL as a result
    * If 'in_handle' or 'index' are invalid, NULL will be returned
    * This will copy the element into the vector, either using the 'copy function' provieded when the vector was created or using memcpy if no copy function was provided
       - If the 'copy function' is not NULL, it will be called as so:          fn_copy( internal_elem_addr, in_elem );
       - If the 'copy function' is NULL, then memcpy will be called as so:     memcpy( internal_elem_addr, in_elem, elem_size );
*/
void* axk_vector_insert( struct axk_vector_t* in_handle, uint64_t index, void* in_elem );

/*
    axk_vector_insert_range
    * Inserts multiple elements from a buffer into the vector referred to by 'in_handle'
    * Valid inputs for index are still [0 to axk_vector_count( in_handle )] as is with 'axk_vector_insert'
    * The returned pointer is a pointer to the first element inserted into the vectors internal sotrage buffer
    * 'elem_count' is the number of elements to insert, therefore, 'in_elem' must point to a buffer that is at least 'elem_count * elem_size' large is a copy is desired
    * If 'in_elem' is NULL, the space allocated within the vector will not have any data copied into it, and can be done manually by the caller if desired
    * See 'axk_vector_insert' for more information on how insertion works in general
    * If 'in_handle', 'index', or 'elem_count' are invalid, NULL is returned
    * This will copy each element into the vector either will multiple invocation of the 'copy function' provided when the vector was created, or through a single memcpy,
      which will occur if no 'copy function' was provided
        - If the 'copy function' is not NULL, it will be called as so:          for each elem: fn_copy( internal_elem_begin + i * elem_size, in_elem + i * elem_size );
        - If the 'copy function' is NULL, then memcpy will be called as so:     memcpy( internal_elem_begin, in_elem, elem_count * elem_size );
*/
void* axk_vector_insert_range( struct axk_vector_t* in_handle, uint64_t index, void* in_elem, uint64_t elem_count );

/*
    axk_vector_erase
    * Erases an entry in the vector referred to by 'in_handle', at the specified index
    * Erased elements have the 'finalize function' called for each, to perform any finalization required by the element type. The 'finalize function' is provided
      by the call to 'axk_vector_create'. If no 'finalize function' is provided, then no special handling occurs and the underlying memory is simply freed from the heap
    * 'index' has the valid range of [0 to axk_vector_count( in_handle ) - 1] if the vector is not empty
    * This call will ALWAYS fail when the vector is empty
    * Any space left after the erasure of an element, is filled by shifting procceeding elements down
    * If 'in_handle' or 'index' are invalid, then false is returned, otherwise, true is returned
*/
bool axk_vector_erase( struct axk_vector_t* in_handle, uint64_t index );

/*
    axk_vector_erase_range
    * Erases multiple, consecutive elements from the vector referred to by 'in_handle'
    * 'index' is the starting (inclusive) index of the desired range, and 'count' is the number to erase
    * If the provided range is invalid (i.e. empty vector or, index + count > axk_vector_count( in_handle ) ), then false is returned, otherwise true is returned
    * If 'in_handle' doesnt refer to a valid vector, false is returned as well
    * If this function returns false, then no deletion of any elements will occur
    * See 'axk_vector_erase' for more information on how the indivisual erasures will occur, and how the 'finalize function' works
*/
bool axk_vector_erase_range( struct axk_vector_t* in_handle, uint64_t index, uint64_t count );

/*
    axk_vector_resize
    * Changes the size of the vector referred to by 'in_handle', by either growing or shrinking the number of active elements
    * Any active elements that are removed as a result of this call are erased. To see how elements are erased see 'axk_vector_erase'
    * Any active elements that are created as a result of this call, are not zero filled
    * If 'in_handle' is invalid, then this call will simply return
*/
void axk_vector_resize( struct axk_vector_t* in_handle, uint64_t new_size );

/*
    axk_vector_shrink
    * Checks if the vector is able to be shrunk from its current size, freeing up unused memory
    * If the vector is shrunk, it will still have enough space to fit at least the active elements
    * If 'in_handle' is invalid, then the function will return with no effect
*/
void axk_vector_shrink( struct axk_vector_t* in_handle );

/*
    axk_vector_reserve
    * Reserves additional space in the vector for future elements, but does not change the number of active elements
    * If 'new_capacity' is less than or equal to the current capacity (axk_vector_capacity), then this function will have no effect
    * If 'new_capacity' is greater than the current capacity, additional space will be allocated to increase capacity to AT LEAST 'new_capacity' 
    * The capacity increase might allocate extra space past the 'new_capacity' requested in some cases, so after this call, the capacity returned
      by a call to 'axk_vector_capacity' will be GREATER THAN OR EQUAL to 'new_capacity'
*/
void axk_vector_reserve( struct axk_vector_t* in_handle, uint64_t new_capacity );

/*
    axk_vector_push_back
    * Inserts an element into the vector referred to by 'in_handle', at the back of the list
    * For more information on insertions, look at 'axk_vector_insert'
*/
void* axk_vector_push_back( struct axk_vector_t* in_handle, void* in_elem );

/*
    axk_vector_push_front
    * Inserts an element into the vector referred to by 'in_handle', at the front of the list
    * For more information on insertions, look at 'axk_vector_insert'
*/
void* axk_vector_push_front( struct axk_vector_t* in_handle, void* in_elem );

/*
    axk_vector_pop_back
    * Removes an element from the back of the vector referred to by 'in_handle'
    * If the vector is empty, the function simply returns
    * For more information on element deletion, look at 'axk_vector_erase'
*/
void axk_vector_pop_back( struct axk_vector_t* in_handle );

/*
    axk_vector_pop_front
    * Removes an element from the front of the vector referred to by 'in_handle'
    * If the vector is empty, the function simply returns
    * For more information on element deletion, look at 'axk_vector_erase'
*/
void axk_vector_pop_front( struct axk_vector_t* in_handle );

/*
    axk_vector_get_back
    * Equivalent to 'axk_vector_index( in_handle, axk_vector_count( in_handle ) - 1UL )'
*/
void* axk_vector_get_back( struct axk_vector_t* in_handle );

/*
    axk_vector_get_front
    * Equivalent to 'axk_vector_index( in_handle, 0UL )'
*/
void* axk_vector_get_front( struct axk_vector_t* in_handle );

/*
    axk_vector_compare
    * Compares the active elements of the two vectors, and works similar to the C library function, memcmp
    * Initially, this function will check if either of the vectors are NULL, using the following logic:
       lhs != NULL && rhs == NULL then return 1
       lhs == NULL && rhs != NULL then return -1
       lhs == NULL && rhs == NULL then return 0
    * If 'fn_compare' is NULL, then 'memcmp' is called using the active element portion of each buffer
        - If 'memcmp' returns a value != 0, then that value will be returned 
    * If 'fn_compare' is not NULL, loop through N active elements, where N = min( lhs.count, rhs.count )
        - If fn_compare( lhs[ i ], rhs[ i ] ) != 0, then that value will be returned
    * If we either return 0 from 'memcmp', or each call to 'fn_compare' returns 0, then perform the following logic:
        - If 'lhs.count == rhs.count' then return 0
        - If 'lhs.count > rhs.count' then return 1
        - If 'lhs.count < rhs.count' then return -1
*/
bool axk_vector_compare( struct axk_vector_t* lhs, struct axk_vector_t* rhs, int( *fn_compare )( void*, void* ) );



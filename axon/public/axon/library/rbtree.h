/*==============================================================
    Axon Kernel - Red-Black Binary Search Tree
    2021, Zachary Berry
    axon/public/axon/library/rbtree.h
==============================================================*/

#pragma once
#include "axon/kernel/kernel.h"
#include "axon/library/vector.h"

/*
    rbtree_node_t (STRUCTURE)
    * A node within a red-black binary search tree
    * Data storage is a little complex, because we dont have templates like in C++
    * We can either store the data on the heap, and have the 'delete node' function automatically delete the heap data
    * OR, we can store the data directly in the node if it is 8-bytes or less
*/
struct axk_rbtree_node_t
{
    uint64_t key;
    uint8_t color;
    struct axk_rbtree_node_t* parent;
    struct axk_rbtree_node_t* child[ 2 ];
};

/*
    axk_rbtree_t (STRUCTURE)
    * An instance of a red-black binary search tree
*/
struct axk_rbtree_t
{
    struct axk_rbtree_node_t* root;
    struct axk_rbtree_node_t* leftmost;
    uint64_t count;
    uint64_t elem_size;
    void( *fn_finalize )( void* );
    void( *fn_copy )( void*, void* );
};

/*
    axk_rbtree_iterator_t (STRUCTURE)
    * Used to iterate through a red-black binary search tree
*/
struct axk_rbtree_iterator_t
{
    struct axk_rbtree_t* tree;
    struct axk_rbtree_node_t* node;
    struct axk_vector_t stack;
};

/*
    axk_rbtree_create_iterator
    * Creates a new rbtree iterator
    * Ensure that all iterators are properly 'created' and 'destroyed' when no longer needed to avoid memory leaks!
    * If 'in_iter' is NULL, the call will cause the kernel to panic
*/
void axk_rbtree_create_iterator( struct axk_rbtree_iterator_t* in_iter );

/*
    axk_rbtree_destroy_iterator
    * Destroys a previously created rbtree iterator
    * Ensure this is called on all iterators before they go out of scope to avoid memory leaks!
    * If 'in_iter' is NULL, the function will have no effect
*/
void axk_rbtree_destroy_iterator( struct axk_rbtree_iterator_t* in_iter );

/*
    axk_rbtree_read_iterator
    * Reads the value pointed to by an rbtree iterator
    * If the iterator handle is NULL, doesnt refer to a valid iterator pointing to a valid element, this will return 'NULL'
    * Otherwise, this will return a pointer to the data in the node pointed to by the iterator
*/
void* axk_rbtree_read_iterator( struct axk_rbtree_iterator_t* in_iter );

/*
    axk_rbtree_create
    * Creates a new rbtree container, and sets 'in_handle' to refer to this new container
    * If 'in_handle' already refers to an existing rbtree, that rbtree will be destroyed and the handle reassigned to the new rbtree
    * You MUST provide an accurate element size, this indicates how much room to allocate per element in each node
    * If the type you are inserting into the rbtree requires special handling on destruction, pass a pointer to the destructor function
      into this call through 'finalize_func'. It will be automatically invoked when the element is removed or the rbtree is destroyed
    * If you do not pass a finalize function, the rbtree will only free the memory taken up by the element, and not perform special handling
    * 'copy_func' allows a function to be provided, which will be invoked whenever an element needs to be copied INTO the rbtree, this can be due to
       calls to 'axk_rbtree_copy', 'axk_rbtree_insert', 'axk_rbtree_insert_or_update' or 'axk_rbtree_update'
    * If 'copy_func' is NULL, then copying will be performed by 'memcpy' automatically, copying "#elem_size" bytes from the source pointer to the internal storage
*/
void axk_rbtree_create( struct axk_rbtree_t* in_handle, uint64_t elem_size, void( *copy_func )( void*, void* ), void( *finalize_func )( void* ) );

/*
    axk_rbtree_destroy
    * Destroys an rbtree that the specified handle points to
    * If the handle doesnt point to an rbtree, then the call will have no effect
    * If a 'finalize function' was provided when the rbtree was created, it will be called on each element before the memory that element occupies
      is freed from the heap, allowing more complex structures to be stored within the container
    * After this is called on a handle, it can be reused to create another rbtree with different parameters if desired
*/
void axk_rbtree_destroy( struct axk_rbtree_t* in_handle );

/*
    axk_rbtree_copy
    * Copies one rbtree instance, to another rbtree handle
    * If the 'dest_handle' already refers to an rbtree, that rbtree will be destroyed and recreated with the same parameters as the source rbtree
    * If 'source_handle' is NULL, then 'dest_handle' will just be destroyed, and not recreated
    * Each element in the rbtree, will be copied to the destination tree. This copy can either by..
       1. A raw memory copy (memcpy) if a 'copy_func' was not provided on creation of the tree
       2. An element by element copy if a 'copy_func' was provided, by invoking this function on each element
*/
void axk_rbtree_copy( struct axk_rbtree_t* source_handle, struct axk_rbtree_t* dest_handle );

/*
    axk_rbtree_clear
    * Deletes all nodes from an rbtree thats referred to by the input handle (in_handle)
    * If the rbtree is NULL or not created, then this call will have no effect
    * If a 'finalize_func' was provided when the rbtree was created, it will be invoked on each element before being freed from the heap
    * Otherwise, the memory occupied by the element will just be freed from the heap with no special handling beforehand
*/
void axk_rbtree_clear( struct axk_rbtree_t* in_handle );

/*
    axk_rbtree_count
    * Gets the total number of nodes in an rbtree 
*/
uint64_t axk_rbtree_count( struct axk_rbtree_t* in_handle );

/*
    axk_rbtree_search
    * Searches through an rbtree for the specified key and the associated node
    * If rbtree handle is NULL, or doesnt refer to a valid rbtree, then 'false' will be returned
    * Otherwise, the search will occur, and if a node is found with the specified key, 'out_iter' will be set to point to the node and 'true' will be returned
    * If a node with the specified key was NOT found, then 'out_iter' will be cleared and 'false' will be returned
*/
bool axk_rbtree_search( struct axk_rbtree_t* in_handle, uint64_t in_key, struct axk_rbtree_iterator_t* out_iter );

/*
    axk_rbtree_search_fast
    * Searches through the rbtree for a node with the specified key using an algorithm more simple than the full search function
    * This function, if successful, will return a pointer to the data contained within the node associated with the provided key
    * If the rbtree handle is NULL, or doesnt refer to a valid rbtree, then 'NULL' will be returned
    * If the key is not found within the rbtree, then 'NULL' will be returned
*/
void* axk_rbtree_search_fast( struct axk_rbtree_t* in_handle, uint64_t in_key );

/*
    axk_rbtree_insert_or_update
    * Attempts to insert a new value into an rbtree, but, if a node already exists with the given key, the existing element will be destroyed and the new one copied in
    * After the operation completes, the address of the inserted/updated element is returned, or 'NULL' if the function failed
    * If the provided rbtree handle is NULL, or doesnt refer to a valid rbtree, then 'NULL' is returned
    * If 'in_elem' is NULL, then 'NULL' will be returned, and the function will have no effect on the tree
    * If there already is existing data associated with the specified key, it will be destroyed if a 'finalize_func' was provided during the creation of the rbtree
      otherwise, no destruction will be performed because the memory will simply be overwritten when the assignment occurs
    * In all cases where the provided rbtree handle is valid, a copy operation will occur to copy the element into the new/existing node. If a 'copy_func' was provided
      when the rbtree was created, it will be invoked to copy the element into the node, otherwise, a memcpy will occur using 'elem_size' as the number of bytes to copy
*/
void* axk_rbtree_insert_or_update( struct axk_rbtree_t* in_handle, uint64_t key, void* in_elem );

/*
    axk_rbtree_insert
    * Inserts a new value into an rbtree using the specified key, but, if an element already exists with the given key, the call will return 'NULL'
    * If the function succeeds, then the address of the inserted data will be returned
    * If the rbtree handle is NULL, or doesnt refer to a valid rbtree, then 'NULL' is returned
    * If 'in_elem' is NULL, then the call will return 'NULL' and there will be no effect on the rbtree
    * The new element will be copied into the created node either using the 'copy_func' provided when the rbtree was created, or if no 'copy_func' was
      provided, then a call to 'memcpy' will be made, where the number of bytes copied is the 'elem_size' of the rbtree
*/
void* axk_rbtree_insert( struct axk_rbtree_t* in_handle, uint64_t key, void* in_elem );

/*
    axk_rbtree_update
    * Searches for a node within the rbtree with the specified key, and assigns a new element to that node
    * If the function succeeds, then the address of the updated data will be returned
    * If the rbtree handle is NULL, or doesnt refer to a valid rbtree, then 'NULL' is returned
    * If there isnt a node with the given key, or if 'in_elem' is NULL, then the call will return 'NULL' and there will be no effect on the rbtree
    * If a node is found with the provided key, it is first destroyed if a 'finalize_func' was provided during the creation of the rbtree, otherwise its simply overwritten
    * Then, the new element is copied into the node, either using the 'copy_func' provided during the creation of the rbtree, or using 'memcpy', where the number
      of bytes that are copied is equal to 'elem_size' also provided during rbtree creation
*/
void* axk_rbtree_update( struct axk_rbtree_t* in_handle, uint64_t key, void* in_elem );

/*
    axk_rbtree_erase
    * Erases a node from the rbtree using an iterator to indicate which node to erase
    * If the rbtree handle is NULL, or doesnt refer to a valid rbtree, then 'false' will be returned
    * If the iterator is NULL, or doesnt refer to a valid element in the rbtree, then 'false' will be returned
    * If the iterator points to an element from a different rbtree than the provided one, 'false' will also be returned
    * Once the element is identified, it will either be deleted using 'finalize_func' provided when the rbtree was created, or, the memory that
      the element resides within will simply be freed from the heap
*/
bool axk_rbtree_erase( struct axk_rbtree_t* in_handle, struct axk_rbtree_iterator_t* in_pos );

/*
    axk_rbtree_erase_key
    * Searches for a node within an rbtree with the given key and erases it
    * If a node is found with the given key, then this function will return 'true', otherwise 'false'
    * If the provided rbtree handle is NULL, or if it doesnt refer to a valid rbtree, then 'false' will be returned
    * See 'axk_rbtree_erase' for more information on how elements are erased
*/
bool axk_rbtree_erase_key( struct axk_rbtree_t* in_handle, uint64_t key );

/*
    axk_rbtree_leftmost
    * Very quickly retrieves the cached leftmost node of the rbtree
    * if the rbtree handle is NULL, or doesnt refer to a valid rbtree, then 'NULL' will be returned
    * If the rbtree is empty, 'NULL' will also be returned
    * Otherwise, this function will return a pointer to the element data associated with the leftmost node, and 'out_key' will have the associated key written to it
    * If 'out_key' is NULL, the function will perform as normal, except the leftmost nodes key will not be written to it, only the data returned
*/
void* axk_rbtree_leftmost( struct axk_rbtree_t* in_handle, uint64_t* out_key );

/*
    axk_rbtree_next
    * Gets the next node in the sequence of the rbtree using the provided iterator
    * If the iterator handle doesnt already refer to an element within an rbtree, then 'false' will be returned
    * If the current node pointed to by the iterator is the final node in the tree, then 'false' will be returned and the iterator will not be updated
    * Otherwise, the iterator will be updated to point to the next element in the rbtree
    * Iterators can be reused with different rbtrees, by calling 'axk_rbtree_begin' on the new tree
*/
bool axk_rbtree_next( struct axk_rbtree_iterator_t* in_iter );

/*
    axk_rbtree_begin
    * Sets up an iterator to be associated with the given rbtree and to point to the first node (with the lowest key)
    * If the rbtree handle is NULL, or doesnt refer to a valid rbtree, then 'false' will be returned
    * If the iterator handle is NULL, or doesnt refer to a valid iterator, then 'false' will also be returned
*/
bool axk_rbtree_begin( struct axk_rbtree_t* in_handle, struct axk_rbtree_iterator_t* in_iter );

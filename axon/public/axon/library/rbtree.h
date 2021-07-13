/*==============================================================
    Axon Kernel - Red-Black Binary Search Tree
    2021, Zachary Berry
    axon/public/axon/library/rbtree.h
==============================================================*/

#pragma once
#include "axon/config.h"
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
    union
    {
        void* heap_data;
        uint64_t inline_data;
    };
    
    uint64_t key;
    uint8_t color;
    bool b_heap_data;
    struct axk_rbtree_node_t* left;
    struct axk_rbtree_node_t* right;
    struct axk_rbtree_node_t* parent;
};

/*
    axk_rbtree_t (STRUCTURE)
    * An instance of a red-black binary search tree
*/
struct axk_rbtree_t
{
    struct axk_rbtree_node_t* root;
    uint64_t count;
};

/*
    axk_rbtree_iterator_t (STRUCTURE)
    * Used to iterate through a red-black binary search tree
*/
struct axk_rbtree_iterator_t
{
    struct axk_rbtree_node_t* node;
    struct axk_vector_t stack;
};

/*
    axk_rbtree_iterator_init
    * Initializes a new iterator
*/
void axk_rbtree_iterator_init( struct axk_rbtree_iterator_t* in_iter );

/*
    axk_rbtree_iterator_destroy
    * Destroys an iterator
*/
void axk_rbtree_iterator_destroy( struct axk_rbtree_iterator_t* in_iter );

/*
    axk_rbtree_init
    * Creates a new red-black tree instance
*/
void axk_rbtree_init( struct axk_rbtree_t* in_tree );

/*
    axk_rbtree_destroy
    * Destroys a red-black tree instance
*/
void axk_rbtree_destroy( struct axk_rbtree_t* in_tree );

/*
    axk_rbtree_copy
    * Copies an RBTree instance to a seperate RBTree
*/
void axk_rbtree_copy( struct axk_rbtree_t* source_tree, struct axk_rbtree_t* dest_tree );

/*
    axk_rbtree_clear
    * Clears the contents of an RBTree
*/
void axk_rbtree_clear( struct axk_rbtree_t* in_tree );

/*
    axk_rbtree_count
    * Gets the total number of elements in an RBTree
*/
uint64_t axk_rbtree_count( struct axk_rbtree_t* in_tree );

/*
    axk_rbtree_search
    * Searches for a specified key in the tree
    * Returns a full iterator
*/
bool axk_rbtree_search( struct axk_rbtree_t* in_tree, uint64_t in_key, struct axk_rbtree_iterator_t* out_iter );

/*
    axk_rbtree_search_fast
    * Saerches for a specified key in the tree
    * Returns a node pointer, null if not found
*/
struct axk_rbtree_node_t* axk_rbtree_search_fast( struct axk_rbtree_t* in_tree, uint64_t in_key );

/*
    axk_rbtree_insert_or_update
    * Attempts to insert a new value, but if the key is already in use, it will update
      the existing entry with the new value
    * Returns the affected (or new) node
    * The data can either be a pointer to heap data, or 8-bytes of inlined data
    * 'b_did_update' will be set to true if the operation resulted in a node update instaed of an insert
*/
bool axk_rbtree_insert_or_update( struct axk_rbtree_t* in_tree, uint64_t key, uint64_t data, bool b_heap_data, bool* b_did_update );

/*
    axk_rbtree_insert
    * Attempts to insert a new value, if the key already exists the call will fail
    * The data can either be a pointer to heap data, or 8-bytes of ilnlined data
    * If the existing data was allocated on the heap, it will be freed automatically
*/
bool axk_rbtree_insert( struct axk_rbtree_t* in_tree, uint64_t key, uint64_t data, bool b_heap_data );

/*
    axk_rbtree_update
    * Attempts to update an entry in the rbtree with a new value
    * If the existing value is allocated on the heap, it will be freed
    * If there is no entry with the specified key, this will return false
*/
bool axk_rbtree_update( struct axk_rbtree_t* in_tree, uint64_t key, uint64_t data, bool b_heap_data );

/*
    axk_rbtree_erase
    * Erases a node from an rbtree
    * If the data contained in this node is from the heap, it will be freed from the heap
    * 'in_pos' will be updated with an iterator pointing to the next element in the tree
    * Returns false if theres a failure
*/
bool axk_rbtree_erase( struct axk_rbtree_t* in_tree, struct axk_rbtree_iterator_t* in_pos );

/*
    axk_rbtree_next
    * Gets the next node in an rbtree
    * If the passed in node iterator isnt valid, or its the last in the tree, it will return false
    * 'in_iter' will be updated to point to the next entry in the sequence
*/
bool axk_rbtree_next( struct axk_rbtree_iterator_t* in_iter );

/*
    axk_rbtree_begin
    * Gets the first node in an rbtree
    * If the tree is empty, will return false
*/
bool axk_rbtree_begin( struct axk_rbtree_t* in_tree, struct axk_rbtree_iterator_t* out_iter );

/*
    axk_rbtree_get_value
    * Gets the data associated with an RBTree entry from an iterator
*/
bool axk_rbtree_get_value( struct axk_rbtree_iterator_t* in_iter, uint64_t* out_value );

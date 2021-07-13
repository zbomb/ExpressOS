/*==============================================================
    Axon Kernel - Red-Black Binary Search Tree
    2021, Zachary Berry
    axon/source/library/rbtree.c
==============================================================*/

#include "axon/library/rbtree.h"
#include "axon/library/vector.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"

// Testing.. Delete this!
static uint32_t test = 0;

/*
    Constants
*/
#define COLOR_BLACK  0
#define COLOR_RED    1
#define DIR_LEFT     0
#define DIR_RIGHT    1


/*
    Helper Functions
*/
uint8_t node_get_dir_from_parent( struct axk_rbtree_node_t* in_node )
{
    if( in_node == NULL || in_node->parent == NULL ) { return DIR_LEFT; }
    return( in_node->parent->left == in_node ? DIR_LEFT : DIR_RIGHT );
}

struct axk_rbtree_node_t* node_get_grandparent( struct axk_rbtree_node_t* in_node )
{
    if( in_node == NULL || in_node->parent == NULL ) { return NULL; }
    return in_node->parent->parent;
}


struct axk_rbtree_node_t* node_get_sibiling( struct axk_rbtree_node_t* in_node )
{
    if( in_node == NULL || in_node->parent == NULL ) { return NULL; }
    return( node_get_dir_from_parent( in_node ) == DIR_LEFT ? in_node->parent->right : in_node->parent->left );
}


struct axk_rbtree_node_t* node_get_uncle( struct axk_rbtree_node_t* in_node )
{
    return( in_node == NULL ? NULL : node_get_sibiling( in_node->parent ) );
}


struct axk_rbtree_node_t* node_get_nephew( struct axk_rbtree_node_t* in_node, bool b_close )
{
    if( in_node == NULL || in_node->parent == NULL ) { return NULL; }
    uint8_t dir = node_get_dir_from_parent( in_node );
    struct axk_rbtree_node_t* sibiling = ( dir == DIR_LEFT ? in_node->parent->right : in_node->parent->left );
    if( sibiling == NULL ) { return NULL; }
    if( b_close )
    {
        return( dir == DIR_LEFT ? sibiling->left : sibiling->right );
    }
    else
    {
        return( dir == DIR_LEFT ? sibiling->right : sibiling->left );
    }
}


struct axk_rbtree_node_t* tree_rotate( struct axk_rbtree_t* in_tree, struct axk_rbtree_node_t* p, uint8_t dir )
{
    // Validate parameters
    if( in_tree == NULL || p == NULL || dir > DIR_RIGHT ) { return NULL; }

    struct axk_rbtree_node_t* g = p->parent;
    struct axk_rbtree_node_t* s = ( dir == DIR_LEFT ? p->right : p->left );
    if( s == NULL ) { return NULL; }

    struct axk_rbtree_node_t* c = ( dir == DIR_LEFT ? s->left : s->right );
    if( dir == DIR_LEFT )
    {
        p->right = c;
    }
    else
    {
        p->left = c;
    }

    if( c != NULL ) { c->parent = p; }
    if( dir == DIR_LEFT )
    {
        s->left = p;
    }
    else
    {
        s->right = p;
    }

    p->parent = s;
    s->parent = g;
    
    if( g != NULL )
    {
        if( p == g->right )
        {
            g->right = s;
        }
        else
        {
            g->left = s;
        }
    }
    else
    {
        in_tree->root = s;
    }

    // Return the new subtree root
    return s;
}



struct axk_rbtree_node_t* tree_rotate_left( struct axk_rbtree_t* in_tree, struct axk_rbtree_node_t* in_node )
{
    return tree_rotate( in_tree, in_node, DIR_LEFT );
}


struct axk_rbtree_node_t* tree_rotate_right( struct axk_rbtree_t* in_tree, struct axk_rbtree_node_t* in_node )
{
    return tree_rotate( in_tree, in_node, DIR_RIGHT );
}


bool tree_delete_node( struct axk_rbtree_t* in_tree, struct axk_rbtree_node_t* in_node )
{
    if( in_tree == NULL || in_node == NULL ) { return false; }

    // First lets handle a root node without children
    if( in_node == in_tree->root && in_node->left == NULL && in_node->right == NULL )
    {
        if( in_tree->root->b_heap_data && in_tree->root->heap_data ) 
        {
            free( in_tree->root->heap_data );
        }

        free( in_tree->root );
        in_tree->root = NULL;
        in_tree->count = 0UL;
        return true;
    }

    // Next lets handle any cases with two children
    if( in_node->left != NULL && in_node->right != NULL )
    {
        // Were going to find the lowest element in the right subtree from the current node
        struct axk_rbtree_node_t* min_right = in_node->right;
        while( min_right->left != NULL )
        {
            min_right = min_right->left;
        }

        // Now, we are going to swap the current node with the 'min_right' node
        struct axk_rbtree_node_t* tmp_parent    = in_node->parent;
        struct axk_rbtree_node_t* tmp_left      = in_node->left;
        struct axk_rbtree_node_t* tmp_right     = in_node->right;
        uint8_t tmp_color                       = in_node->color;

        uint8_t tdir = node_get_dir_from_parent( in_node );

        in_node->parent      = min_right->parent;
        in_node->left        = min_right->left;
        in_node->right       = min_right->right;
        in_node->color       = min_right->color;
        
        uint8_t ndir = node_get_dir_from_parent( min_right );
        if( ndir == DIR_LEFT )
        {
            in_node->parent->left = in_node;
        }
        else
        {
            in_node->parent->right = in_node;
        }

        min_right->parent   = tmp_parent;
        min_right->left     = tmp_left;
        min_right->right    = tmp_right;
        min_right->color    = tmp_color;

        if( tdir == DIR_LEFT )
        {
            min_right->parent->left = min_right;
        }
        else
        {
            min_right->parent->right = min_right;
        }

        min_right->left->parent     = min_right;
        min_right->right->parent    = min_right;
        if( in_node->left != NULL )   { in_node->left->parent = in_node; }
        if( in_node->right != NULL )  { in_node->right->parent = in_node; }
    }

    // DEBUG DEBUG DEBUG DEBUG DEBUG
    bool b_second = ( ( test++ ) == 1 );

    // If N is a red node...
    if( in_node->color == COLOR_RED )
    {
        // Delete this node..
        uint8_t dir = node_get_dir_from_parent( in_node );
        if( dir == DIR_LEFT )
        {
            in_node->parent->left = NULL;
        }
        else
        {
            in_node->parent->right = NULL;
        }

        if( in_node->b_heap_data && in_node->heap_data != NULL )
        {
            free( in_node->heap_data );
        }
        
        free( in_node );
        in_tree->count--;
        return true;
    }
    else
    {
        // If N is a black node...
        // We can either have a red-child, or no valid children at all
        // We can simply replace the current node with this red child, and paint it black
        struct axk_rbtree_node_t* child = ( in_node->left != NULL ? in_node->left : in_node->right );
        if( child != NULL )
        {
            child->color = COLOR_BLACK;
            if( in_node->parent != NULL )
            {
                uint8_t ndir = node_get_dir_from_parent( in_node );
                if( ndir == DIR_LEFT )
                {
                    in_node->parent->left = child;
                }
                else
                {
                    in_node->parent->right = child;
                }
            }
            else
            {
                in_tree->root = child;
            }

            if( in_node->b_heap_data && in_node->heap_data != NULL )
            {
                free( in_node->heap_data );
            }

            free( in_node );
            in_tree->count--;
            return true;
        }
    }

    //if( b_second ) { axk_halt(); }

    // Any deletions not handled by this point will require a rebalance of the tree, and more complex handling
    struct axk_rbtree_node_t* n     = in_node;
    struct axk_rbtree_node_t* p     = n->parent;
    struct axk_rbtree_node_t* s     = NULL;
    struct axk_rbtree_node_t* c     = NULL;
    struct axk_rbtree_node_t* d     = NULL;

    uint8_t dir = node_get_dir_from_parent( n );

    if( dir == DIR_LEFT ) 
    {
        p->left = NULL;
    }
    else
    {
        p->right = NULL;
    }

    // Delete the node itself
    if( n->b_heap_data && n->heap_data != NULL )
    {
        free( n->heap_data );
    }

    free( n );
    in_tree->count--;
    n = NULL;

    goto loop_start;

    do
    {
        dir = node_get_dir_from_parent( n );
        loop_start:
        if( dir == DIR_LEFT ) 
        { s = p->right; } 
        else 
        { s = p->left; }
        if( s->color == COLOR_RED )
        {
            tree_rotate( in_tree, p, dir );
            p->color = COLOR_RED;
            s->color = COLOR_BLACK;
            s = c;
            
            if( dir == DIR_LEFT )
            { d = s->right; }
            else
            { d = s->left; }

            if( d != NULL && d->color == COLOR_RED )
            {
                tree_rotate( in_tree, p, dir );
                s->color = p->color;
                p->color = COLOR_BLACK;
                d->color = COLOR_BLACK;
                return true;
            }
            
            if( dir == DIR_LEFT )
            { c = s->left; }
            else
            { c = s->right; }

            if( c != NULL && c->color == COLOR_RED )
            {
                tree_rotate( in_tree, s, 1 - dir );
                s->color = COLOR_RED;
                c->color = COLOR_BLACK;
                d = s;
                s = c;

                tree_rotate( in_tree, p, dir );
                s->color = p->color;
                p->color = COLOR_BLACK;
                d->color = COLOR_BLACK;
                return true;
            }

            s->color = COLOR_RED;
            p->color = COLOR_BLACK;

            return true;
        }

        if( dir == DIR_LEFT )
        { d = s->right; }
        else
        { d = s->left; }

        if( d != NULL && d->color == COLOR_RED )
        {
            tree_rotate( in_tree, p, dir );
            s->color = p->color;
            p->color = COLOR_BLACK;
            d->color = COLOR_BLACK;
            return true;
        }

        if( dir == DIR_LEFT )
        { c = s->left; }
        else
        { c = s->right; }

        if( c != NULL && c->color == COLOR_RED )
        {
            tree_rotate( in_tree, s, 1 - dir );
            s->color = COLOR_RED;
            c->color = COLOR_BLACK;
            d = s;
            s = c;

            tree_rotate( in_tree, p, dir );
            s->color = p->color;
            p->color = COLOR_BLACK;
            d->color = COLOR_BLACK;
            return true;
        }

        if( p->color == COLOR_RED )
        {
            s->color = COLOR_RED;
            p->color = COLOR_BLACK;
            return true;
        }

        s->color = COLOR_RED;
        n = p;
        p = n->parent;

    } while( p != NULL );

    return true;
}


/*
    Function Implementations
*/
void axk_rbtree_iterator_init( struct axk_rbtree_iterator_t* in_iter )
{
    in_iter->node = NULL;
    axk_vector_create( &( in_iter->stack ), sizeof( void* ) );
}


void axk_rbtree_iterator_destroy( struct axk_rbtree_iterator_t* in_iter )
{
    in_iter->node = NULL;
    axk_vector_destroy( &( in_iter->stack ) );
}


void axk_rbtree_init( struct axk_rbtree_t* in_tree )
{
    if( in_tree == NULL ) { return; }

    in_tree->root   = NULL;
    in_tree->count  = 0UL;
}


void axk_rbtree_destroy( struct axk_rbtree_t* in_tree )
{
    axk_rbtree_clear( in_tree );
}


void axk_rbtree_copy( struct axk_rbtree_t* source, struct axk_rbtree_t* dest )
{
    // Clear destination tree
    if( dest == NULL ) { return; }
    axk_rbtree_clear( dest );

    // Check for empty source tree
    if( source == NULL || source->root == NULL )
    {
        dest->root = NULL;
        return;
    }

    // Copy over the root node
    dest->root          = (struct axk_rbtree_node_t*)malloc( sizeof( struct axk_rbtree_node_t ) );
    *( dest->root )     = *( source->root );
    dest->root->left    = NULL;
    dest->root->right   = NULL;
    dest->root->parent  = NULL;
    dest->count         = 1UL;

    // Now, were going to iterate through the source tree and copy the nodes into the destination tree
    struct axk_rbtree_node_t* source_node   = source->root;
    struct axk_rbtree_node_t* dest_node     = dest->root;

    // Create a 'stack' to keep track of where we have been, with a capacity of the theoretical max height of a balanced RB tree of the current size
    struct axk_vector_t stack;
    int max_height = log2_64( source->count );
    axk_vector_create_with_capacity( &stack, sizeof( void* ), ( max_height <= 1 ? 1UL : (uint64_t)( max_height ) ) * 2UL );
    
    // Push source, then dest node onto the stack
    axk_vector_push_back( &stack, &( source_node ) );
    axk_vector_push_back( &stack, &( dest_node ) );

    while( source_node->left != NULL )
    {
        source_node = source_node->left;

        // Copy the next node
        dest_node->left = (struct axk_rbtree_node_t*) malloc( sizeof( struct axk_rbtree_node_t ) );
        memcpy( (void*)( dest_node->left ), (void*)( source_node ), sizeof( struct axk_rbtree_node_t ) );

        dest_node->left->parent     = dest_node;
        dest_node                   = dest_node->left;
        dest_node->left             = NULL;
        dest_node->right            = NULL;
        

        dest->count++;

        // Push it onto the stack
        axk_vector_push_back( &stack, &( source_node ) );
        axk_vector_push_back( &stack, &( dest_node ) );
    }

    // Now we can enter the main iterator loop
    while( true )
    {
        // Check for a right sibiling
        if( source_node->right != NULL )
        {
            // Copy node to dest tree
            dest_node->right = (struct axk_rbtree_node_t*) malloc( sizeof( struct axk_rbtree_node_t ) );
            memcpy( (void*)( dest_node->right ), (void*)( source_node->right ), sizeof( struct axk_rbtree_node_t ) );

            dest_node->right->parent    = dest_node;
            dest_node                   = dest_node->right;
            dest_node->left             = NULL;
            dest_node->right            = NULL;
            source_node                 = source_node->right;
            dest->count++;

            // Pop the last node off the stack, since we dont need to go back to it again
            axk_vector_pop_back( &stack );
            axk_vector_pop_back( &stack );

            // Push the next node onto the stack
            axk_vector_push_back( &stack, &source_node );
            axk_vector_push_back( &stack, &dest_node );

            // Now, we need to navigate down the leftmost path from this node to a leaf
            while( source_node->left != NULL )
            {
                // Copy this node in
                dest_node->left = (struct axk_rbtree_node_t*) malloc( sizeof( struct axk_rbtree_node_t ) );
                memcpy( (void*)( dest_node->left ), (void*)( source_node->left ), sizeof( struct axk_rbtree_node_t ) );

                dest_node->left->parent     = dest_node;
                dest_node                   = dest_node->left;
                dest_node->left             = NULL;
                dest_node->right            = NULL;
                source_node                 = source_node->left;
                dest->count++;

                // Push to stack
                axk_vector_push_back( &stack, &source_node );
                axk_vector_push_back( &stack, &dest_node );
            }
        }
        else
        {
            // Pop the current node off the stack
            axk_vector_pop_back( &stack );
            axk_vector_pop_back( &stack );

            // If there are no more nodes on the stack, then we finished
            uint64_t elem_count = axk_vector_count( &stack );
            if( elem_count < 2UL ) { break; }

            // We processed both the left and right descendants of the current node, so lets go back
            // to the next parent that hasnt been fully checked yet
            dest_node       = axk_vector_index( &stack, elem_count - 1UL );
            source_node     = axk_vector_index( &stack, elem_count - 2UL );
        }
    }

    // Clean up the vector we used
    axk_vector_destroy( &stack );
}


void axk_rbtree_clear( struct axk_rbtree_t* in_tree )
{
    // To clear: we use an iterative approach, to avoid recursion
    // Similar to how 'rbiter_t' works, or how 'copy' works but with some changes
    // Check for null tree
    if( in_tree->root == NULL )
    {
        return;
    }

    // Set current node to the root, add it to the stack
    struct axk_rbtree_node_t* node = in_tree->root;

    struct axk_vector_t stack;
    int max_height = log2_64( in_tree->count );
    axk_vector_create_with_capacity( &stack, sizeof( void* ), ( max_height <= 1 ? 1UL : (uint64_t)( max_height ) ) * 2UL );
    axk_vector_push_back( &stack, &node );

    // Navigate as far down left as possible
    while( node->left != NULL )
    {
        node = node->left;
        axk_vector_push_back( &stack, &node );
    }

    // Now that were at the lowest left leaf, we can enter the main loop
    while( true )
    {
        // First, check if theres a node on the right
        if( node->right != NULL )
        {
            // Go to the right node, store on the stack
            node = node->right;
            axk_vector_push_back( &stack, &node );

            // Traverse down left again
            while( node->left != NULL )
            {
                node = node->left;
                axk_vector_push_back( &stack, &node );
            }
        }
        else
        {
            // There should be NO children for this node
            // So we can delete it, and then go back to the parent node
            // First, remove this node from its parent
            if( node->parent != NULL )
            {
                if( node->parent->right == node )   { node->parent->right = NULL; }
                if( node->parent->left == node )    { node->parent->left = NULL; }
            }

            free( (void*) node );
            axk_vector_pop_back( &stack );

            // Check if we have emptied the stack
            if( axk_vector_count( &stack ) == 0UL )
            {
                break;
            }

            node = *( (struct axk_rbtree_node_t**)axk_vector_get_back( &stack ) );
        }
    }

    // Clear the root and count
    in_tree->root   = NULL;
    in_tree->count  = 0UL;
}


uint64_t axk_rbtree_count( struct axk_rbtree_t* in_tree )
{
    if( in_tree == NULL ) { return 0UL; }
    return in_tree->count;
}


bool axk_rbtree_search( struct axk_rbtree_t* in_tree, uint64_t in_key, struct axk_rbtree_iterator_t* out_iter )
{
    if( in_tree == NULL || in_tree->root == NULL || out_iter == NULL ) { return false; }

    // Clear the iterator
    axk_vector_clear( &( out_iter->stack ) );
    out_iter->node      = NULL;

    struct axk_rbtree_node_t* pos = in_tree->root;

    // Now, start at the root and search
    while( pos != NULL )
    {
        // Check if we found the target node
        if( in_key == pos->key ) { break; }

        // Determine if were on the left or right side of the current node
        struct axk_rbtree_node_t* next_node = ( in_key < pos->key ? pos->left : pos->right );

        if( next_node != NULL )
        {
            // Add the current node to the stack before moving to the next
            axk_vector_push_back( &( out_iter->stack ), &pos );
        }

        pos = next_node;
    }

    // If the current node is null, then we couldnt find the entry
    if( pos == NULL )
    {
        return false;
    }
    else
    {
        out_iter->node = pos;
        return true;
    }
}


struct axk_rbtree_node_t* axk_rbtree_search_fast( struct axk_rbtree_t* in_tree, uint64_t in_key )
{
    if( in_tree == NULL || in_tree->root == NULL ) { return NULL; }

    struct axk_rbtree_node_t* pos = in_tree->root;
    while( pos != NULL )
    {
        if( in_key == pos->key ) { return pos; }
        pos = ( in_key < pos->key ) ? pos->left : pos->right;
    }

    return NULL;
}


bool axk_rbtree_insert_or_update( struct axk_rbtree_t* in_tree, uint64_t key, uint64_t data, bool b_heap_data, bool* b_did_update )
{
    *b_did_update = false;

    // Find where to insert this node, or an existing node to update
    struct axk_rbtree_node_t* parent    = NULL;
    uint8_t dir                         = DIR_LEFT;

    parent = in_tree->root;
    while( parent != NULL )
    {
        if( key < parent->key )
        {
            dir = DIR_LEFT;
            if( parent->left == NULL ) { break; }
            parent = parent->left;
        }
        else if( key > parent->key )
        {
            dir = DIR_RIGHT;
            if( parent->right == NULL ) { break; }
            parent = parent->right;
        }
        else
        {
            *b_did_update = true;
            
            // Free the current value if needed
            if( parent->b_heap_data && parent->heap_data != NULL )
            {
                free( parent->heap_data );
            }

            parent->inline_data     = data;
            parent->b_heap_data     = b_heap_data;

            return true;
        }
    }

    // We found where to insert this node, there wasnt an existing node
    // Create new node
    struct axk_rbtree_node_t* new_node = (struct axk_rbtree_node_t*) malloc( sizeof( struct axk_rbtree_node_t ) );

    new_node->left          = NULL;
    new_node->right         = NULL;
    new_node->parent        = NULL;
    new_node->inline_data   = data;
    new_node->b_heap_data   = b_heap_data;
    new_node->color         = COLOR_RED;
    new_node->parent        = parent;
    new_node->key           = key;
    in_tree->count++;

    if( parent != NULL )
    {
        if( dir == DIR_LEFT )     { parent->left = new_node; }
        if( dir == DIR_RIGHT )    { parent->right = new_node; }
    }
    else
    {
        in_tree->root = new_node;
        return true;
    }

    // Begin rebalance
    struct axk_rbtree_node_t* grand = NULL;
    struct axk_rbtree_node_t* uncle = NULL;

    do
    {
        if( parent->color == COLOR_BLACK )
        {
            break;
        }

        grand = parent->parent;
        if( grand == NULL )
        {
            parent->color = COLOR_BLACK;
            break;
        }

        dir = node_get_dir_from_parent( parent );
        if( dir == DIR_LEFT ) 
        {
            uncle = grand->right;
        }
        else
        {
            uncle = grand->left;
        }

        if( uncle == NULL || uncle->color == COLOR_BLACK )
        {
            if( ( dir == DIR_LEFT && new_node == parent->right ) ||
                ( dir == DIR_RIGHT && new_node == parent->left ) )
            {
                tree_rotate( in_tree, parent, dir );
                new_node    = parent;
                parent  = ( dir == DIR_LEFT ? grand->left : grand->right );
            }

            tree_rotate( in_tree, grand, 1 - dir );
            parent->color   = COLOR_BLACK;
            grand->color    = COLOR_RED;

            break;
        }

        parent->color   = COLOR_BLACK;
        uncle->color    = COLOR_BLACK;
        grand->color    = COLOR_RED;
        new_node        = grand;
        parent          = new_node->parent;

    } while( parent != NULL );

    return true;
}


bool axk_rbtree_insert( struct axk_rbtree_t* in_tree, uint64_t key, uint64_t data, bool b_heap_data )
{
    if( in_tree == NULL ) { return false; }

    // Find the place to insert this node, to do this, search for the key
    struct axk_rbtree_node_t* parent = NULL;
    uint8_t dir = DIR_LEFT;

    parent = in_tree->root;
    while( parent != NULL )
    {
        // If the current parent node doesnt contain a child in the correct direction, we found the parent node and final direction
        if( key < parent->key )
        {
            // We should be on the right subtree of the current node
            dir = DIR_LEFT;
            if( parent->left == NULL ) { break; }
            parent = parent->left;
        }
        else if( key > parent->key )
        {
            // We should be on the left subtree of the current node
            dir = DIR_RIGHT;
            if( parent->right == NULL ) { break; }
            parent = parent->right;
        }
        else
        {
            // key == parent->key
            return false;
        }
    }

    // Create the new node
    struct axk_rbtree_node_t* new_node = (struct axk_rbtree_node_t*) malloc( sizeof( struct axk_rbtree_node_t ) );

    new_node->left          = NULL;
    new_node->right         = NULL;
    new_node->parent        = NULL;
    new_node->inline_data   = data;
    new_node->b_heap_data   = b_heap_data;
    new_node->color         = COLOR_RED;
    new_node->key           = key;

    // Now we have the parent node and direction we should be from it, so lets do the insertion
    new_node->parent = parent;
    in_tree->count++;

    if( parent == NULL )
    {
        in_tree->root = new_node;
        return true;
    }
    else if( dir == DIR_LEFT )
    {
        parent->left = new_node;
    }
    else
    {
        parent->right = new_node;
    }

    // Begin rebalance
    struct axk_rbtree_node_t* grand     = NULL;
    struct axk_rbtree_node_t* uncle     = NULL;
    struct axk_rbtree_node_t* node      = new_node;

    do
    {
        if( parent->color == COLOR_BLACK )
        {
            return true;
        }

        grand = parent->parent;
        if( grand == NULL )
        {
            parent->color = COLOR_BLACK;
            return true;
        }

        dir = node_get_dir_from_parent( parent );
        if( dir == DIR_LEFT ) 
        {
            uncle = grand->right;
        }
        else
        {
            uncle = grand->left;
        }

        if( uncle == NULL || uncle->color == COLOR_BLACK )
        {
            if( ( dir == DIR_LEFT && node == parent->right ) ||
                ( dir == DIR_RIGHT && node == parent->left ) )
            {
                tree_rotate( in_tree, parent, dir );
                node    = parent;
                parent  = ( dir == DIR_LEFT ? grand->left : grand->right );
            }

            tree_rotate( in_tree, grand, 1 - dir );
            parent->color   = COLOR_BLACK;
            grand->color    = COLOR_RED;

            return true;
        }

        parent->color   = COLOR_BLACK;
        uncle->color    = COLOR_BLACK;
        grand->color    = COLOR_RED;
        node            = grand;
        parent          = node->parent;

    } while( parent != NULL );

    return true;
}


bool axk_rbtree_update( struct axk_rbtree_t* in_tree, uint64_t key, uint64_t data, bool b_heap_data )
{
    // Find the node if possible
    if( in_tree == NULL ) { return false; }

    struct axk_rbtree_node_t* node = axk_rbtree_search_fast( in_tree, key );
    if( node == NULL ) { return false; }

    // Free exisitng value if needed
    if( node->b_heap_data && node->heap_data != NULL )
    {
        free( node->heap_data );
    }

    node->inline_data = data;
    node->b_heap_data = b_heap_data;

    return true;
}


bool axk_rbtree_erase( struct axk_rbtree_t* in_tree, struct axk_rbtree_iterator_t* in_pos )
{
    if( in_tree == NULL || in_pos == NULL || in_pos->node == NULL ) { return false; }
    
    struct axk_rbtree_node_t* node = in_pos->node;
    axk_rbtree_next( in_pos );

    if( !tree_delete_node( in_tree, node ) )
    {
        in_pos->node = NULL;
        axk_vector_clear( &( in_pos->stack ) );

        return false;
    }

    return true;
}


bool axk_rbtree_next( struct axk_rbtree_iterator_t* in_iter )
{
    if( in_iter == NULL || in_iter->node == NULL ) { return false; }

    // 1. Check if the node has a 'right' child
    if( in_iter->node->right != NULL )
    {
        // 2a. Go to that node, push it onto the stack
        in_iter->node = in_iter->node->right;
        axk_vector_push_back( &( in_iter->stack ), &(in_iter->node) );

        // 3. Traverse down the chain of 'left' chldren
        while( in_iter->node->left != NULL )
        {
            in_iter->node = in_iter->node->left;
            axk_vector_push_back( &( in_iter->stack ), &(in_iter->node) );
        }

        // 4. Pop the last node off the stack (node already set to this value), this is going to be the next node in our iteration
        axk_vector_pop_back( &( in_iter->stack ) );
        return true;
    }
    else
    {
        // 2b. Pop the next node off the stack and visit it
        //  - If the stack is empty, then we hit the end of the tree
        if( axk_vector_count( &( in_iter->stack ) ) == 0UL )
        {
            in_iter->node = NULL;
            return false;
        }
        else
        {
            in_iter->node = *( (struct axk_rbtree_node_t**)axk_vector_get_back( &( in_iter->stack ) ) );
            axk_vector_pop_back( &( in_iter->stack ) );
            return true;
        }
    }
}


bool axk_rbtree_begin( struct axk_rbtree_t* in_tree, struct axk_rbtree_iterator_t* out_iter )
{
    if( in_tree == NULL || in_tree->root == NULL || out_iter == NULL ) { return false; }

    if( axk_vector_count( &( out_iter->stack ) ) != 0UL )
    {
        axk_vector_clear( &( out_iter->stack ) );
    }

    out_iter->node = in_tree->root;
    while( out_iter->node->left != NULL )
    {
        axk_vector_push_back( &( out_iter->stack ), &( out_iter->node ) );
        out_iter->node = out_iter->node->left;
    }

    return true;
}


bool axk_rbtree_get_value( struct axk_rbtree_iterator_t* in_iter, uint64_t* out_value )
{
    if( in_iter == NULL || out_value == NULL || in_iter->node == NULL ) { return false; }

    *out_value = in_iter->node->inline_data;
    return true;
}



/*==============================================================
    Axon Kernel - Red-Black Binary Search Tree
    2021, Zachary Berry
    axon/source/library/rbtree.c
==============================================================*/

#include "axon/library/rbtree.h"
#include "axon/library/vector.h"
#include "axon/panic.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"

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
uint8_t node_get_dir_from_parent( struct axk_rbtree_node_t* n )
{
    return( n == n->parent->child[ DIR_RIGHT ] ? DIR_RIGHT : DIR_LEFT );
}

struct axk_rbtree_node_t* node_get_parent( struct axk_rbtree_node_t* n )
{
    return( n == NULL ? NULL : n->parent );
}

struct axk_rbtree_node_t* node_get_grandparent( struct axk_rbtree_node_t* n )
{
    return n->parent == NULL ? NULL : n->parent->parent;
}


struct axk_rbtree_node_t* node_get_sibiling( struct axk_rbtree_node_t* n )
{
    return n->parent->child[ 1 - node_get_dir_from_parent( n ) ];
}


struct axk_rbtree_node_t* node_get_uncle( struct axk_rbtree_node_t* n )
{
    return node_get_sibiling( n->parent );
}


struct axk_rbtree_node_t* node_get_nephew( struct axk_rbtree_node_t* n, bool b_close )
{
    uint8_t dir = node_get_dir_from_parent( n );
    struct axk_rbtree_node_t* s = n->parent->child[ 1 - dir ];
    return( b_close ? s->child[ dir ] : s->child[ 1 - dir ] );
}


struct axk_rbtree_node_t* tree_rotate( struct axk_rbtree_t* t, struct axk_rbtree_node_t* p, uint8_t dir )
{
    struct axk_rbtree_node_t* g = p->parent;
    struct axk_rbtree_node_t* s = p->child[ 1 - dir ];
    struct axk_rbtree_node_t* c;

    c = s->child[ dir ];
    p->child[ 1 - dir ] = c;
    if( c != NULL ) { c->parent = p; }
    s->child[ dir ] = p;
    p->parent = s;
    s->parent = g;

    if( g != NULL )
    {
        g->child[ ( p == g->child[ DIR_RIGHT ] ) ? DIR_RIGHT : DIR_LEFT ] = s;
    }
    else
    {
        t->root = s;
    }

    return s;
}



struct axk_rbtree_node_t* tree_rotate_left( struct axk_rbtree_t* t, struct axk_rbtree_node_t* n )
{
    return tree_rotate( t, n, DIR_LEFT );
}


struct axk_rbtree_node_t* tree_rotate_right( struct axk_rbtree_t* t, struct axk_rbtree_node_t* n )
{
    return tree_rotate( t, n, DIR_RIGHT );
}


void tree_insert_node( struct axk_rbtree_t* t, struct axk_rbtree_node_t* p, struct axk_rbtree_node_t* n, uint8_t dir )
{
    struct axk_rbtree_node_t* g;
    struct axk_rbtree_node_t* u;

    n->color                = COLOR_RED;
    n->child[ DIR_LEFT ]    = NULL;
    n->child[ DIR_RIGHT ]   = NULL;
    n->parent               = p;

    if( p == NULL )
    {
        t->root = n;
        return;
    }

    p->child[ dir ] = n;

    do
    {
        if( p->color == COLOR_BLACK )
        {
            return;
        }

        if( ( g = node_get_parent( p ) ) == NULL )
        {
            p->color = COLOR_BLACK;
            return;
        }

        dir = node_get_dir_from_parent( p );
        u = g->child[ 1 - dir ];
        if( u == NULL || u->color == COLOR_BLACK )
        {
            if( n == p->child[ 1 - dir ] ) 
            {
                tree_rotate( t, p, dir );
                n = p;
                p = g->child[ dir ];
            }

            tree_rotate( t, g, 1 - dir );
            p->color = COLOR_BLACK;
            g->color = COLOR_RED;
            return;
        }

        p->color = COLOR_BLACK;
        u->color = COLOR_BLACK;
        g->color = COLOR_RED;
        n = g;

    } while( ( p = n->parent ) != NULL );

    return;
}


void node_destroy( struct axk_rbtree_node_t* n, void( *fn_finalize )( void* ) )
{
    if( n != NULL )
    {
        if( fn_finalize != NULL )
        {
            fn_finalize( (void*)( (uint8_t*)( n ) + sizeof( struct axk_rbtree_node_t ) ) );
        }

        free( n );
    }
}


void node_copy( struct axk_rbtree_node_t* dest, struct axk_rbtree_node_t* source, void( *fn_copy )( void*, void* ), uint64_t elem_size )
{
    // Copy the fields we can, i.e. non-pointers
    dest->key                   = source->key;
    dest->color                 = source->color;
    dest->parent                = NULL;
    dest->child[ DIR_LEFT ]     = NULL;
    dest->child[ DIR_RIGHT ]    = NULL;

    // Copy the value either using a raw memcpy or the provided copy function
    if( fn_copy == NULL )
    {
        memcpy( (void*)( (uint8_t*)( dest ) + sizeof( struct axk_rbtree_node_t ) ), (void*)( (uint8_t*)( source ) + sizeof( struct axk_rbtree_node_t ) ), elem_size );
    }
    else
    {
        fn_copy( (void*)( (uint8_t*)( dest ) + sizeof( struct axk_rbtree_node_t ) ), (void*)( (uint8_t*)( source ) + sizeof( struct axk_rbtree_node_t ) ) );
    }
}


void tree_delete( struct axk_rbtree_t* t, struct axk_rbtree_node_t* n )
{
    // First, check if the node is the tree root without children
    if( n == t->root && n->child[ DIR_LEFT ] == NULL && n->child[ DIR_RIGHT ] == NULL )
    {
        t->root     = NULL;
        t->count    = 0UL;

        node_destroy( n, t->fn_finalize );
        return;
    }

    // Next, check if the node has two non-null children
    if( n->child[ DIR_LEFT ] != NULL && n->child[ DIR_RIGHT ] != NULL )
    {
        // Were going to find the lowest element in the right subtree from the current node
        struct axk_rbtree_node_t* min_right = n->child[ DIR_RIGHT ];
        while( min_right->child[ DIR_LEFT] != NULL )
        {
            min_right = min_right->child[ DIR_LEFT ];
        }

        // Now, we are going to swap the current node with the 'min_right' node
        struct axk_rbtree_node_t* tmp_parent    = n->parent;
        struct axk_rbtree_node_t* tmp_left      = n->child[ DIR_LEFT ];
        struct axk_rbtree_node_t* tmp_right     = n->child[ DIR_RIGHT ];
        uint8_t tmp_color                       = n->color;

        uint8_t tdir = node_get_dir_from_parent( n );

        n->parent               = min_right->parent;
        n->child[ DIR_LEFT ]    = min_right->child[ DIR_LEFT ];
        n->child[ DIR_RIGHT ]   = min_right->child[ DIR_RIGHT ];
        n->color                = min_right->color;
        
        uint8_t ndir = node_get_dir_from_parent( min_right );
        if( ndir == DIR_LEFT )
        {
            n->parent->child[ DIR_LEFT ] = n;
        }
        else
        {
            n->parent->child[ DIR_RIGHT ] = n;
        }

        min_right->parent               = tmp_parent;
        min_right->child[ DIR_LEFT ]    = tmp_left;
        min_right->child[ DIR_RIGHT ]   = tmp_right;
        min_right->color                = tmp_color;

        if( tdir == DIR_LEFT )
        {
            min_right->parent->child[ DIR_LEFT ] = min_right;
        }
        else
        {
            min_right->parent->child[ DIR_RIGHT ] = min_right;
        }

        min_right->child[ DIR_LEFT ]->parent     = min_right;
        min_right->child[ DIR_RIGHT ]->parent    = min_right;
        if( n->child[ DIR_LEFT ] != NULL )   { n->child[ DIR_LEFT ]->parent = n; }
        if( n->child[ DIR_RIGHT ] != NULL )  { n->child[ DIR_RIGHT ]->parent = n; }
    }

    if( n->color == COLOR_RED )
    {
        // Delete this node..
        uint8_t dir = node_get_dir_from_parent( n );
        n->parent->child[ dir ] = NULL;

        node_destroy( n, t->fn_finalize );
        t->count--;
        return;
    }
    else
    {
        // If N is a black node...
        // We can either have a red-child, or no valid children at all
        // We can simply replace the current node with this red child, and paint it black
        struct axk_rbtree_node_t* ch = ( n->child[ DIR_LEFT ] != NULL ? n->child[ DIR_LEFT ] : n->child[ DIR_RIGHT ] );
        if( ch != NULL )
        {
            ch->color = COLOR_BLACK;
            if( n->parent != NULL )
            {
                n->parent->child[ node_get_dir_from_parent( n ) ] = ch;
                ch->parent = n->parent;
            }
            else
            {
                t->root = ch;
                ch->parent = NULL;
            }

            node_destroy( n, t->fn_finalize );
            t->count--;
            return;
        }
    }

    struct axk_rbtree_node_t* p = n->parent;
    uint8_t dir;
    struct axk_rbtree_node_t* s;
    struct axk_rbtree_node_t* c;
    struct axk_rbtree_node_t* d;

    dir = node_get_dir_from_parent( n );
    p->child[ dir ] = NULL;
    node_destroy( n, t->fn_finalize );

    goto start_loop;

    do
    {
        dir = node_get_dir_from_parent( n );
        start_loop:
    
        s = p->child[ 1 - dir ];
        if( s->color == COLOR_RED )
        {
            goto delete_3;
        }

        d = s->child[ 1 - dir ];
        if( d != NULL && d->color == COLOR_RED )
        {
            goto delete_6;
        }

        c = s->child[ dir ];
        if( c != NULL && c->color == COLOR_RED )
        {
            goto delete_5;
        }

        if( p->color == COLOR_RED )
        {
            goto delete_4;
        }

        s->color = COLOR_RED;
        n = p;

    } while( ( p = n->parent ) != NULL );

    return;

delete_3:

    tree_rotate( t, p, dir );
    p->color = COLOR_RED;
    s->color = COLOR_BLACK;
    s = c;

    d = s->child[ 1 - dir ];
    if( d != NULL && d->color == COLOR_RED )
    {
        goto delete_6;
    }

    c = s->child[ dir ];
    if( c != NULL && c->color == COLOR_RED )
    {
        goto delete_5;
    }

delete_4:

    s->color = COLOR_RED;
    p->color = COLOR_BLACK;
    return;

delete_5:

    tree_rotate( t, s, 1 - dir );
    s->color = COLOR_RED;
    c->color = COLOR_BLACK;
    d = s;
    s = c;

delete_6:

    tree_rotate( t, p, dir );
    s->color = p->color;
    p->color = COLOR_BLACK;
    d->color = COLOR_BLACK;
    return;

}


void tree_cache_leftmost( struct axk_rbtree_t* t )
{
    if( t == NULL ) { return; }
    if( t->root == NULL ) 
    { 
        t->leftmost = NULL; 
        return; 
    }

    struct axk_rbtree_node_t* n = t->root;
    while( n->child[ DIR_LEFT ] != NULL )
    {
        n = n->child[ DIR_LEFT ];
    }

    t->leftmost = n;
}

/*
    Function Implementations
*/
void axk_rbtree_create_iterator( struct axk_rbtree_iterator_t* in_iter )
{
    if( in_iter == NULL )
    {
        axk_panic( "Kernel Containers: attempt to create rbtree iterator with NULL handle" );
    }

    in_iter->node = NULL;
    in_iter->tree = NULL;

    AXK_ZERO_MEM( in_iter->stack );
    axk_vector_create( &( in_iter->stack ), sizeof( void* ), NULL, NULL );
}


void axk_rbtree_destroy_iterator( struct axk_rbtree_iterator_t* in_iter )
{
    if( in_iter != NULL )
    {
        in_iter->node = NULL;
        in_iter->tree = NULL;
        axk_vector_destroy( &( in_iter->stack ) );
    }
}


void* axk_rbtree_read_iterator( struct axk_rbtree_iterator_t* in_iter )
{
    if( in_iter == NULL || in_iter->node == NULL ) { return NULL; }
    return (void*)( (uint8_t*)( in_iter->node ) + sizeof( struct axk_rbtree_iterator_t ) );
}


void axk_rbtree_create( struct axk_rbtree_t* in_handle, uint64_t elem_size, void( *copy_func )( void*, void* ), void( *finalize_func )( void* ) )
{
    if( in_handle == NULL ) { axk_panic( "Kernel Containers: attempt to create an rbtree with a NULL handle" ); }
    if( elem_size == 0UL )  { axk_panic( "Kernel Containers; attempt to create an rbtree with an invalid element size" ); }

    // Check if the handle already refers to an instance
    if( in_handle->root != NULL )
    {
        axk_rbtree_destroy( in_handle );
    }

    in_handle->root         = NULL;
    in_handle->leftmost     = NULL;
    in_handle->count        = 0UL;
    in_handle->elem_size    = elem_size;
    in_handle->fn_copy      = copy_func;
    in_handle->fn_finalize  = finalize_func;
}


void axk_rbtree_destroy( struct axk_rbtree_t* in_handle )
{
    if( in_handle != NULL )
    {
        axk_rbtree_clear( in_handle );

        in_handle->root         = NULL;
        in_handle->leftmost     = NULL;
        in_handle->count        = 0UL;
        in_handle->elem_size    = 0UL;
        in_handle->fn_finalize  = NULL;
        in_handle->fn_copy      = NULL;
    }
}


void axk_rbtree_copy( struct axk_rbtree_t* source_handle, struct axk_rbtree_t* dest_handle )
{
    // Destroy the current destination tree
    if( dest_handle == NULL ) { return; }
    axk_rbtree_destroy( dest_handle );

    // Recreate the destination tree using the parameters from the source tree
    if( source_handle == NULL ) { return; }
    axk_rbtree_create( dest_handle, source_handle->elem_size, source_handle->fn_copy, source_handle->fn_finalize );

    // Copy the root node into the destination tree
    dest_handle->root   = (struct axk_rbtree_node_t*) malloc( sizeof( struct axk_rbtree_node_t ) + source_handle->elem_size );
    dest_handle->count  = 1UL;

    node_copy( dest_handle->root, source_handle->root, source_handle->fn_copy, source_handle->elem_size );

    if( source_handle->leftmost == source_handle->root )
    {
        dest_handle->leftmost = dest_handle->root;
    }

    // Now, were going to iterate through the source tree and copy the nodes into the destination tree
    struct axk_rbtree_node_t* source_node   = source_handle->root;
    struct axk_rbtree_node_t* dest_node     = dest_handle->root;

    // Create a 'stack' to keep track of where we have been, with a capacity of the theoretical max height of a balanced RB tree of the current size
    struct axk_vector_t stack;
    AXK_ZERO_MEM( stack );

    int max_height = log2_64( source_handle->count );
    axk_vector_create_with_capacity( &stack, sizeof( void* ), ( max_height <= 1 ? 1UL : (uint64_t)( max_height ) ) * 2UL, NULL, NULL );
    
    // Push source, then dest node onto the stack
    axk_vector_push_back( &stack, &( source_node ) );
    axk_vector_push_back( &stack, &( dest_node ) );

    while( source_node->child[ DIR_LEFT ] != NULL )
    {
        source_node = source_node->child[ DIR_LEFT ];

        // Copy the next node
        dest_node->child[ DIR_LEFT ] = (struct axk_rbtree_node_t*) malloc( sizeof( struct axk_rbtree_node_t ) + source_handle->elem_size );
        node_copy( dest_node->child[ DIR_LEFT ], source_node, source_handle->fn_copy, source_handle->elem_size );

        if( source_node == source_handle->leftmost )
        {
            dest_handle->leftmost = dest_node->child[ DIR_LEFT ];
        }

        dest_node->child[ DIR_LEFT ]->parent    = dest_node;
        dest_node                               = dest_node->child[ DIR_LEFT ];
        dest_handle->count++;

        // Push it onto the stack
        axk_vector_push_back( &stack, &( source_node ) );
        axk_vector_push_back( &stack, &( dest_node ) );
    }

    // Now we can enter the main iterator loop
    while( true )
    {
        // Check for a right sibiling
        if( source_node->child[ DIR_RIGHT ] != NULL )
        {
            // Copy node to dest tree
            dest_node->child[ DIR_RIGHT ] = (struct axk_rbtree_node_t*) malloc( sizeof( struct axk_rbtree_node_t ) + source_handle->elem_size );
            node_copy( dest_node->child[ DIR_RIGHT ], source_node->child[ DIR_RIGHT ], source_handle->fn_copy, source_handle->elem_size );

            if( source_node->child[ DIR_RIGHT ] == source_handle->leftmost )
            {
                dest_handle->leftmost = dest_node->child[ DIR_RIGHT ];
            }

            dest_node->child[ DIR_RIGHT ]->parent       = dest_node;
            dest_node                                   = dest_node->child[ DIR_RIGHT ];
            source_node                                 = source_node->child[ DIR_RIGHT ];
            dest_handle->count++;

            // Pop the last node off the stack, since we dont need to go back to it again
            axk_vector_pop_back( &stack );
            axk_vector_pop_back( &stack );

            // Push the next node onto the stack
            axk_vector_push_back( &stack, &source_node );
            axk_vector_push_back( &stack, &dest_node );

            // Now, we need to navigate down the leftmost path from this node to a leaf
            while( source_node->child[ DIR_LEFT ] != NULL )
            {
                // Copy this node in
                dest_node->child[ DIR_LEFT ] = (struct axk_rbtree_node_t*) malloc( sizeof( struct axk_rbtree_node_t ) + source_handle->elem_size );
                node_copy( dest_node->child[ DIR_LEFT ], source_node->child[ DIR_LEFT ], source_handle->fn_copy, source_handle->elem_size );

                if( source_node->child[ DIR_LEFT ] == source_handle->leftmost )
                {
                    dest_handle->leftmost = dest_node->child[ DIR_LEFT ];
                }

                dest_node->child[ DIR_LEFT ]->parent    = dest_node;
                dest_node                               = dest_node->child[ DIR_LEFT ];
                source_node                             = source_node->child[ DIR_LEFT ];
                dest_handle->count++;

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


void axk_rbtree_clear( struct axk_rbtree_t* in_handle )
{
    // Check for NULL handle/tree
    if( in_handle == NULL || in_handle->root == NULL ) { return; }

    // Set current node to the root, add it to the stack
    struct axk_rbtree_node_t* node = in_handle->root;

    struct axk_vector_t stack;
    AXK_ZERO_MEM( stack );
    
    int max_height = log2_64( in_handle->count );
    axk_vector_create_with_capacity( &stack, sizeof( void* ), ( max_height <= 1 ? 1UL : (uint64_t)( max_height ) ) * 2UL, NULL, NULL );
    axk_vector_push_back( &stack, &node );

    // Navigate as far down left as possible
    while( node->child[ DIR_LEFT ] != NULL )
    {
        node = node->child[ DIR_LEFT ];
        axk_vector_push_back( &stack, &node );
    }

    // Now that were at the lowest left leaf, we can enter the main loop
    while( true )
    {
        // First, check if theres a node on the right
        if( node->child[ DIR_RIGHT ] != NULL )
        {
            // Go to the right node, store on the stack
            node = node->child[ DIR_RIGHT ];
            axk_vector_push_back( &stack, &node );

            // Traverse down left again
            while( node->child[ DIR_LEFT ] != NULL )
            {
                node = node->child[ DIR_LEFT ];
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
                if( node->parent->child[ DIR_RIGHT ] == node )   { node->parent->child[ DIR_RIGHT ] = NULL; }
                if( node->parent->child[ DIR_LEFT ] == node )    { node->parent->child[ DIR_LEFT ] = NULL; }
            }

            node_destroy( node, in_handle->fn_finalize );
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
    in_handle->root         = NULL;
    in_handle->leftmost     = NULL;
    in_handle->count        = 0UL;
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
        struct axk_rbtree_node_t* next_node = ( in_key < pos->key ? pos->child[ DIR_LEFT ] : pos->child[ DIR_RIGHT ] );

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


void* axk_rbtree_search_fast( struct axk_rbtree_t* in_handle, uint64_t in_key )
{
    if( in_handle == NULL || in_handle->root == NULL ) { return NULL; }

    struct axk_rbtree_node_t* pos = in_handle->root;
    while( pos != NULL )
    {
        if( in_key == pos->key ) { return (void*)( (uint8_t*)( pos ) + sizeof( struct axk_rbtree_node_t ) ); }
        pos = ( in_key < pos->key ) ? pos->child[ DIR_LEFT ] : pos->child[ DIR_RIGHT ];
    }

    return NULL;
}


void* axk_rbtree_insert_or_update( struct axk_rbtree_t* in_handle, uint64_t key, void* in_elem )
{
    if( in_elem == NULL ) { return NULL; }

    // Find where to insert this node, or an existing node to update
    struct axk_rbtree_node_t* parent    = NULL;
    uint8_t dir                         = DIR_LEFT;

    parent = in_handle->root;
    while( parent != NULL )
    {
        if( key < parent->key )
        {
            dir = DIR_LEFT;
            if( parent->child[ DIR_LEFT ] == NULL ) { break; }
            parent = parent->child[ DIR_LEFT ];
        }
        else if( key > parent->key )
        {
            dir = DIR_RIGHT;
            if( parent->child[ DIR_RIGHT ] == NULL ) { break; }
            parent = parent->child[ DIR_RIGHT ];
        }
        else
        {
            // Free the current value if needed
            if( in_handle->fn_finalize != NULL )
            {
                in_handle->fn_finalize( (void*)( (uint8_t*)( parent ) + sizeof( struct axk_rbtree_node_t ) ) );
            }

            // Copy in the new value
            void* ptr_dest = (void*)( (uint8_t*)( parent ) + sizeof( struct axk_rbtree_node_t ) );
            if( in_handle->fn_copy == NULL )
            {
                memcpy( ptr_dest, in_elem, in_handle->elem_size );
            }
            else
            {
                in_handle->fn_copy( ptr_dest, in_elem );
            }

            return ptr_dest;
        }
    }

    // There wasnt an existing node, so were going to create and insert a new node
    struct axk_rbtree_node_t* new_node = (struct axk_rbtree_node_t*) malloc( sizeof( struct axk_rbtree_node_t ) + in_handle->elem_size );
    new_node->key = key;
    void* ptr_dest = (void*)( (uint8_t*)( new_node ) + sizeof( struct axk_rbtree_node_t ) );

    if( in_handle->fn_copy == NULL )
    {
        memcpy( ptr_dest, in_elem, in_handle->elem_size );
    }
    else
    {
        in_handle->fn_copy( ptr_dest, in_elem );
    }

    tree_insert_node( in_handle, parent, new_node, dir );
    in_handle->count++;

    if( in_handle->leftmost == NULL || key < in_handle->leftmost->key )
    {
        in_handle->leftmost = new_node;
    }

    return ptr_dest;
}


void* axk_rbtree_insert( struct axk_rbtree_t* in_handle, uint64_t key, void* in_elem )
{
    if( in_handle == NULL ) { return NULL; }

    // Find the place to insert this node, to do this, search for the key
    struct axk_rbtree_node_t* parent = in_handle->root;
    uint8_t dir = DIR_LEFT;

    while( parent != NULL )
    {
        // If the current parent node doesnt contain a child in the correct direction, we found the parent node and final direction
        if( key < parent->key )
        {
            // We should be on the right subtree of the current node
            dir = DIR_LEFT;
            if( parent->child[ DIR_LEFT ] == NULL ) { break; }
            parent = parent->child[ DIR_LEFT ];
        }
        else if( key > parent->key )
        {
            // We should be on the left subtree of the current node
            dir = DIR_RIGHT;
            if( parent->child[ DIR_RIGHT ] == NULL ) { break; }
            parent = parent->child[ DIR_RIGHT ];
        }
        else
        {
            // key == parent->key
            return NULL;
        }
    }

    // Create the new node
    struct axk_rbtree_node_t* new_node = (struct axk_rbtree_node_t*) malloc( sizeof( struct axk_rbtree_node_t ) + in_handle->elem_size );
    new_node->key = key;
    void* ptr_dest = (void*)( (uint8_t*)( new_node ) + sizeof( struct axk_rbtree_node_t ) );

    if( in_handle->fn_copy == NULL )
    {
        memcpy( ptr_dest, in_elem, in_handle->elem_size );
    }
    else
    {
        in_handle->fn_copy( ptr_dest, in_elem );
    }

    tree_insert_node( in_handle, parent, new_node, dir );
    in_handle->count++;

    if( in_handle->leftmost == NULL || key < in_handle->leftmost->key )
    {
        in_handle->leftmost = new_node;
    }

    return ptr_dest;
}


void* axk_rbtree_update( struct axk_rbtree_t* in_handle, uint64_t key, void* in_elem )
{
    // Find the node if possible
    if( in_handle == NULL || in_handle->root == NULL ) { return NULL; }

    void* ptr_dest = axk_rbtree_search_fast( in_handle, key );
    if( ptr_dest == NULL ) { return NULL; }

    // Destroy the current element if required
    if( in_handle->fn_finalize != NULL )
    {
        in_handle->fn_finalize( ptr_dest );
    }

    // Copy in the element
    if( in_handle->fn_copy == NULL )
    {
        memcpy( ptr_dest, in_elem, in_handle->elem_size );
    }
    else
    {
        in_handle->fn_copy( ptr_dest, in_elem );
    }

    return ptr_dest;
}


bool axk_rbtree_erase( struct axk_rbtree_t* in_handle, struct axk_rbtree_iterator_t* in_pos )
{
    if( in_handle == NULL || in_handle->root == NULL || in_pos == NULL || in_pos->node == NULL ) { return false; }
    
    struct axk_rbtree_node_t* node = in_pos->node;
    axk_rbtree_next( in_pos );
    tree_delete( in_handle, node );

    if( in_handle->leftmost == node )
    {
        tree_cache_leftmost( in_handle );
    }

    return true;
}


bool axk_rbtree_erase_key( struct axk_rbtree_t* in_handle, uint64_t key )
{
    if( in_handle == NULL || in_handle->root == NULL ) { return false; }

    struct axk_rbtree_node_t* pos = in_handle->root;
    while( pos != NULL )
    {
        if( key == pos->key ) 
        { 
            tree_delete( in_handle, pos );
            if( in_handle->leftmost == pos )
            {
                tree_cache_leftmost( in_handle );
            }

            return true;
        }

        pos = ( key < pos->key ) ? pos->child[ DIR_LEFT ] : pos->child[ DIR_RIGHT ];
    }

    return false;
}


void* axk_rbtree_leftmost( struct axk_rbtree_t* in_handle, uint64_t* out_key )
{
    if( in_handle == NULL || in_handle->root == NULL ) { return NULL; }

    if( in_handle->leftmost == NULL )
    {
        return NULL;
    }
    else
    {
        if( out_key != NULL ) { *out_key = in_handle->leftmost->key; }
        return (void*)( (uint8_t*)( in_handle->leftmost ) + sizeof( struct axk_rbtree_node_t ) );
    }
}


bool axk_rbtree_next( struct axk_rbtree_iterator_t* in_iter )
{
    if( in_iter == NULL || in_iter->node == NULL ) { return false; }

    // 1. Check if the node has a 'right' child
    if( in_iter->node->child[ DIR_RIGHT ] != NULL )
    {
        // 2a. Go to that node, push it onto the stack
        in_iter->node = in_iter->node->child[ DIR_RIGHT ];
        axk_vector_push_back( &( in_iter->stack ), &(in_iter->node) );

        // 3. Traverse down the chain of 'left' chldren
        while( in_iter->node->child[ DIR_LEFT ] != NULL )
        {
            in_iter->node = in_iter->node->child[ DIR_LEFT ];
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
    while( out_iter->node->child[ DIR_LEFT ] != NULL )
    {
        axk_vector_push_back( &( out_iter->stack ), &( out_iter->node ) );
        out_iter->node = out_iter->node->child[ DIR_LEFT ];
    }

    return true;
}



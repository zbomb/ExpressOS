/*==============================================================
    Axon Kernel - Vector Container
    2021, Zachary Berry
    axon/source/library/vector.c
==============================================================*/

#include "axon/library/vector.h"
#include "axon/panic.h"
#include "stdlib.h"
#include "string.h"

/*
    Constants
*/
#define MAX_ELEM_SIZE 4096UL
#define MIN_CAPACITY 4UL
#define CLAMP( _val_, _min_, _max_ ) ( _val_ < _min_ ? _min_ : ( _val_ > _max_ ? _max_ : _val_ ) )

/*
    Helper Function(s)
*/
uint64_t calculate_capacity( struct axk_vector_t* in_vec, uint64_t new_count )
{
    uint64_t count_div      = new_count / 10UL;
    uint64_t growth_div     = (uint64_t)( in_vec->growth_factor / 6 );

    uint64_t ret = ( CLAMP( count_div, 5UL, 1024UL ) * CLAMP( growth_div, 1UL, 10UL ) ) + new_count;
    return( ret < MIN_CAPACITY ? MIN_CAPACITY : ret );
}

uint64_t expand_capacity( struct axk_vector_t* in_vec, uint64_t addtl_count )
{
    // Calculate the new vector capacity
    in_vec->elem_capacity = calculate_capacity( in_vec, addtl_count + in_vec->elem_count );

    // Allocate a new buffer to hold the data
    void* new_buffer = calloc( in_vec->elem_capacity, in_vec->elem_size );

    // Copy in existing elements
    if( in_vec->buffer != NULL )
    {
        memcpy( new_buffer, in_vec->buffer, in_vec->elem_count * in_vec->elem_size );
        free( in_vec->buffer );
    }

    in_vec->buffer = new_buffer;
    return in_vec->elem_capacity;
}

uint64_t check_shrink( struct axk_vector_t* in_vec )
{
    // If the current capacity is at the minimum dont even check
    if( in_vec->elem_capacity <= MIN_CAPACITY || in_vec->buffer == NULL ) { return in_vec->elem_capacity; }

    // Calculate what the capacity 'should' be for the active element count, and if its different than the actual capacity, resize buffer
    uint64_t new_capacity = calculate_capacity( in_vec, in_vec->elem_count );
    if( new_capacity + new_capacity <= in_vec->elem_capacity )
    {
        void* new_buffer = calloc( new_capacity, in_vec->elem_size );
        memcpy( new_buffer, in_vec->buffer, in_vec->elem_count * in_vec->elem_size );
        free( in_vec->buffer );

        in_vec->buffer          = new_buffer;
        in_vec->elem_capacity   = new_capacity;
    }

    return in_vec->elem_capacity;
}

/*
    Function Implementations
*/
void axk_vector_create( struct axk_vector_t* in_handle, uint64_t elem_size )
{
    if( elem_size == 0UL || elem_size > MAX_ELEM_SIZE ) { return; }

    // Calculate 'growth factor' (i.e. factor indicating how much extra capcaity to keep)
    // and set the rest of the fields besides the buffer
    in_handle->elem_size        = elem_size;
    in_handle->elem_count       = 0UL;
    in_handle->growth_factor    = (uint8_t)( 1024UL / elem_size );
    in_handle->growth_factor    = in_handle->growth_factor < 1 ? 1 : ( in_handle->growth_factor > 60 ? 60 : in_handle->growth_factor );
    in_handle->elem_capacity    = calculate_capacity( in_handle, 0UL );

    // Now, lets create the buffer that actually holds the data
    in_handle->buffer = calloc( in_handle->elem_capacity, in_handle->elem_size );
}


void axk_vector_create_with_capacity( struct axk_vector_t* in_handle, uint64_t elem_size, uint64_t in_capacity )
{
    if( elem_size == 0UL || elem_size > MAX_ELEM_SIZE ) { return; }

    // Calculate 'growth factor' (i.e. factor indicating how much extra capcaity to keep)
    // and set the rest of the fields besides the buffer
    in_handle->elem_size        = elem_size;
    in_handle->elem_count       = 0UL;
    in_handle->growth_factor    = (uint8_t)( 1024UL / elem_size );
    in_handle->growth_factor    = in_handle->growth_factor < 1 ? 1 : ( in_handle->growth_factor > 60 ? 60 : in_handle->growth_factor );
    in_handle->elem_capacity    = ( in_capacity < MIN_CAPACITY ? MIN_CAPACITY : in_capacity );

    // Now, lets create the buffer that actually holds the data
    in_handle->buffer = calloc( in_handle->elem_capacity, in_handle->elem_size ); 
}


void axk_vector_destroy( struct axk_vector_t* in_handle )
{
    free( in_handle->buffer );
    in_handle->elem_count       = 0UL;
    in_handle->elem_capacity    = 0UL;
}


void axk_vector_copy( struct axk_vector_t* source, struct axk_vector_t* dest )
{
    if( source == NULL || dest == NULL || source == dest ) { return; }

    // First, if the destination is already initialized, then destroy it
    if( dest->buffer != NULL )
    {
        free( dest->buffer );
    }

    // Allocate and copy over buffer
    dest->buffer = calloc( source->elem_capacity, source->elem_size );
    memcpy( dest->buffer, source->buffer, source->elem_size * source->elem_count );

    // Assign fields
    dest->elem_capacity     = source->elem_capacity;
    dest->elem_count        = source->elem_count;
    dest->elem_size         = source->elem_size;
}


void axk_vector_clear( struct axk_vector_t* in_handle )
{
    if( in_handle->buffer != NULL )
    {
        free( in_handle->buffer );
    }

    in_handle->elem_count       = 0UL;
    in_handle->elem_capacity    = calculate_capacity( in_handle, 0UL );
    in_handle->buffer           = calloc( in_handle->elem_capacity, in_handle->elem_size );
}


void* axk_vector_index( struct axk_vector_t* in_handle, uint64_t index )
{
    // Calculate the position in the buffer this index would be
    if( index >= in_handle->elem_count ) { return NULL; }
    return (void*)( (uint8_t*)( in_handle->buffer ) + ( index * in_handle->elem_size ) );
}


uint64_t axk_vector_count( struct axk_vector_t* in_handle )
{
    return in_handle->elem_count;
}


uint64_t axk_vector_capacity( struct axk_vector_t* in_handle )
{
    return in_handle->elem_capacity;
}


void* axk_vector_buffer( struct axk_vector_t* in_handle )
{
    return in_handle->buffer;
}


bool axk_vector_insert( struct axk_vector_t* in_handle, uint64_t index, void* in_elem )
{
    return axk_vector_insert_range( in_handle, index, in_elem, 1UL );
}


bool axk_vector_insert_range( struct axk_vector_t* in_handle, uint64_t index, void* in_elem, uint64_t elem_count )
{
    if( in_handle == NULL || index > in_handle->elem_count || in_elem == NULL || elem_count == 0UL ) { return false; }

    // Check if we need to increase the size of the buffer
    if( in_handle->elem_count >= in_handle->elem_capacity )
    {
        expand_capacity( in_handle, elem_count );
    }

    // First, check for insertion to the back
    if( index == in_handle->elem_count )
    {
        // Copy element to the back of the vector
        void* ptr_pos = (void*)( (uint8_t*)( in_handle->buffer ) + ( in_handle->elem_size * in_handle->elem_count ) );
        memcpy( ptr_pos, in_elem, in_handle->elem_size * elem_count );

        in_handle->elem_count += elem_count;
        return true;
    }
    else
    {
        // Shift elements up, starting from the target index
        void* copy_start        = (void*)( (uint8_t*)( in_handle->buffer ) + ( in_handle->elem_size * index ) );
        void* copy_dest         = (void*)( (uint8_t*)( copy_start ) + ( in_handle->elem_size * elem_count ) );
        uint64_t copy_length    = in_handle->elem_size * ( in_handle->elem_count - index );

        // Use memmove to ensure we dont get corruped since the two buffer ranges overlap
        memmove( copy_dest, copy_start, copy_length );

        // Insert the new elements
        memcpy( copy_start, in_elem, in_handle->elem_size * elem_count );
        in_handle->elem_count += elem_count;
        return true;
    }
}


bool axk_vector_erase( struct axk_vector_t* in_handle, uint64_t index )
{
    return axk_vector_erase_range( in_handle, index, 1UL );
}


bool axk_vector_erase_range( struct axk_vector_t* in_handle, uint64_t index, uint64_t count )
{
    if( in_handle == NULL ||  count == 0UL || ( index + count ) > in_handle->elem_count ) { return false; }

    // Check if were deleting from the back of the vector
    if( index == ( in_handle->elem_count - count ) )
    {
        // In this case, we dont need to shift down, clear memory, update state and return
        memset( axk_vector_index( in_handle, index ), 0, in_handle->elem_size * count );
        in_handle->elem_count -= count;
    }
    else
    {
        // Shift elements down
        void* copy_dest         = axk_vector_index( in_handle, index );
        void* copy_source       = ( (uint8_t*)( copy_dest ) + ( in_handle->elem_size * count ) );
        uint64_t copy_length    = ( in_handle->elem_count - ( index + count ) ) * in_handle->elem_size;
    
        memmove( copy_dest, copy_source, copy_length );

        // Clear the now vacant elements
        memset( axk_vector_index( in_handle, in_handle->elem_count - count ), 0, in_handle->elem_size * count );
        in_handle->elem_count -= count;
    }

    check_shrink( in_handle );
    return true;
}


void axk_vector_resize( struct axk_vector_t* in_handle, uint64_t new_size )
{
    if( new_size == 0UL )
    {
        axk_vector_clear( in_handle );
    }
    else if( new_size < in_handle->elem_count )
    {
        // Shrink the vector, clear vacant memory
        void* new_end = axk_vector_index( in_handle, new_size );
        uint64_t diff = ( in_handle->elem_count - new_size ) * in_handle->elem_size;

        memset( new_end, 0, diff );
        
        in_handle->elem_count = new_size;
        check_shrink( in_handle );
    }
    else if( new_size > in_handle->elem_count )
    {
        // Grow the vector if needed
        uint64_t diff = new_size - in_handle->elem_count;
        expand_capacity( in_handle, diff );
        in_handle->elem_count = new_size;
    }   
}


void axk_vector_push_back( struct axk_vector_t* in_handle, void* in_elem )
{
    axk_vector_insert_range( in_handle, in_handle->elem_count, in_elem, 1UL );
}


void axk_vector_push_front( struct axk_vector_t* in_handle, void* in_elem )
{
    axk_vector_insert_range( in_handle, 0UL, in_elem, 1UL );
}


void axk_vector_pop_back( struct axk_vector_t* in_handle )
{
    axk_vector_erase_range( in_handle, in_handle->elem_count - 1UL, 1UL );
}


void axk_vector_pop_front( struct axk_vector_t* in_handle )
{
    axk_vector_erase_range( in_handle, 0UL, 1UL );
}


void* axk_vector_get_back( struct axk_vector_t* in_handle )
{
    return axk_vector_index( in_handle, in_handle->elem_count - 1UL );
}


void* axk_vector_get_front( struct axk_vector_t* in_handle )
{
    return axk_vector_index( in_handle, 0UL );
}


bool axk_vector_compare( struct axk_vector_t* lhs, struct axk_vector_t* rhs )
{
    if( lhs == rhs )    { return 0; }
    if( lhs == NULL )   { return 1; }   // && rhs != NULL
    if( rhs == NULL )   { return -1; }  // && lhs != NULL

    if( lhs->elem_size > rhs->elem_size )   { return 1; }
    if( rhs->elem_size > lhs->elem_size )   { return -1; }
    if( lhs->elem_count > rhs->elem_count ) { return 1; }
    if( rhs->elem_count > lhs->elem_count ) { return -1; }

    return memcmp( lhs->buffer, rhs->buffer, lhs->elem_size * lhs->elem_count );
}


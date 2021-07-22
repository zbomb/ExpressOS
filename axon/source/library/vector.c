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
#define MAX_ELEM_SIZE 8192UL
#define MIN_CAPACITY 2UL
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

/*
    Function Implementations
*/
void axk_vector_create( struct axk_vector_t* in_handle, uint64_t elem_size, void( *copy_func )( void*, void* ), void( *finalize_func )( void* ) )
{
    if( in_handle->buffer != NULL )
    {
        axk_vector_destroy( in_handle );
    }

    if( elem_size == 0UL || elem_size > MAX_ELEM_SIZE ) { axk_panic( "Kernel Library: attempt to create vector with invalid element size" ); }

    // Calculate 'growth factor' (i.e. factor indicating how much extra capcaity to keep)
    // and set the rest of the fields besides the buffer
    in_handle->elem_size        = elem_size;
    in_handle->elem_count       = 0UL;
    in_handle->growth_factor    = (uint8_t)( 1024UL / elem_size );
    in_handle->growth_factor    = in_handle->growth_factor < 1 ? 1 : ( in_handle->growth_factor > 60 ? 60 : in_handle->growth_factor );
    in_handle->elem_capacity    = calculate_capacity( in_handle, 0UL );
    in_handle->fn_finalize      = finalize_func;
    in_handle->fn_copy          = copy_func;

    // Now, lets create the buffer that actually holds the data
    in_handle->buffer = calloc( in_handle->elem_capacity, in_handle->elem_size );
}


void axk_vector_create_with_capacity( struct axk_vector_t* in_handle, uint64_t elem_size, uint64_t in_capacity, void( *copy_func )( void*, void* ), void( *finalize_func )( void* ) )
{
    // Check if handle already refers to a vector
    if( in_handle->buffer != NULL )
    {
        axk_vector_destroy( in_handle );
    }

    if( elem_size == 0UL || elem_size > MAX_ELEM_SIZE ) { axk_panic( "Kernel Library: attempt to create vector with invalid element size" ); }

    // Calculate 'growth factor' (i.e. factor indicating how much extra capcaity to keep)
    // and set the rest of the fields besides the buffer
    in_handle->elem_size        = elem_size;
    in_handle->elem_count       = 0UL;
    in_handle->growth_factor    = (uint8_t)( 1024UL / elem_size );
    in_handle->growth_factor    = in_handle->growth_factor < 1 ? 1 : ( in_handle->growth_factor > 60 ? 60 : in_handle->growth_factor );
    in_handle->elem_capacity    = ( in_capacity < MIN_CAPACITY ? MIN_CAPACITY : in_capacity );
    in_handle->fn_finalize      = finalize_func;
    in_handle->fn_copy          = copy_func;

    // Now, lets create the buffer that actually holds the data
    in_handle->buffer = calloc( in_handle->elem_capacity, in_handle->elem_size ); 
}


void axk_vector_destroy( struct axk_vector_t* in_handle )
{
    if( in_handle == NULL ) { return; }

    // Determine if there is a special finalize function for each element
    if( in_handle->fn_finalize != NULL )
    {
        for( uint64_t i = 0; i < in_handle->elem_count; i++ )
        {
            in_handle->fn_finalize( (void*)( (uint8_t*)( in_handle->buffer ) + ( i * in_handle->elem_size ) ) );
        }
    }

    free( in_handle->buffer );

    in_handle->buffer           = NULL;
    in_handle->elem_count       = 0UL;
    in_handle->elem_capacity    = 0UL;
    in_handle->fn_finalize      = NULL;
    in_handle->fn_copy          = NULL;
}


void axk_vector_copy( struct axk_vector_t* dest, struct axk_vector_t* source )
{
    if( dest == NULL ) { return; }
    if( dest->buffer != NULL )
    {
        axk_vector_destroy( dest );
    }

    if( source == NULL || source->buffer == NULL ) { return; }

    // Copy over data fields and allocate buffer
    dest->elem_count        = source->elem_count;
    dest->elem_capacity     = source->elem_capacity;
    dest->elem_size         = source->elem_size;
    dest->growth_factor     = source->growth_factor;
    dest->buffer            = calloc( dest->elem_capacity, dest->elem_size );
    dest->fn_finalize       = source->fn_finalize;
    dest->fn_copy           = source->fn_copy;

    // Now, either copy the raw memory or invoke the copy function for each element
    if( source->fn_copy == NULL )
    {
        memcpy( dest->buffer, source->buffer, source->elem_size * source->elem_count );
    }
    else
    {
        for( uint64_t i = 0; i < dest->elem_count; i++ )
        {
            source->fn_copy( (void*)( (uint8_t*)( dest->buffer ) + ( i * dest->elem_size ) ), (void*)( (uint8_t*)( source->buffer ) + ( i * dest->elem_size ) ) );
        }
    }
}


void axk_vector_move( struct axk_vector_t* dest, struct axk_vector_t* source )
{
    // Destroy contents of destination
    if( dest != NULL )
    {
        axk_vector_destroy( dest );
    }

    // Dont attempt to move if source isnt even valid
    if( source == NULL || source->buffer == NULL ) { return; }

    dest->elem_capacity     = source->elem_capacity;
    dest->elem_count        = source->elem_count;
    dest->elem_size         = source->elem_size;
    dest->fn_finalize       = source->fn_finalize;
    dest->fn_copy           = source->fn_copy;
    dest->growth_factor     = source->growth_factor;
    dest->buffer            = source->buffer;

    source->elem_capacity =  0UL;
    source->elem_count      = 0UL;
    source->elem_size       = 0UL;
    source->fn_finalize     = NULL;
    source->fn_copy         = NULL;
    source->growth_factor   = 0;
    source->buffer          = NULL;
}


void axk_vector_clear( struct axk_vector_t* in_handle )
{
    // Skip if the handle is invalid
    if( in_handle == NULL || in_handle->buffer == NULL ) { return; }

    if( in_handle->fn_finalize != NULL )
    {
        for( uint64_t i = 0; i < in_handle->elem_count; i++ )
        {
            in_handle->fn_finalize( (void*)( (uint8_t*)( in_handle->buffer ) + ( i * in_handle->elem_size ) ) );
        }
    }

    free( in_handle->buffer );

    in_handle->elem_count       = 0UL;
    in_handle->elem_capacity    = calculate_capacity( in_handle, 0UL );
    in_handle->buffer           = calloc( in_handle->elem_capacity, in_handle->elem_size );
}


void* axk_vector_index( struct axk_vector_t* in_handle, uint64_t index )
{
    // Calculate the position in the buffer this index would be
    if( in_handle == NULL || index >= in_handle->elem_count ) { return NULL; }
    return (void*)( (uint8_t*)( in_handle->buffer ) + ( index * in_handle->elem_size ) );
}


uint64_t axk_vector_count( struct axk_vector_t* in_handle )
{
    return in_handle == NULL ? 0UL : in_handle->elem_count;
}


uint64_t axk_vector_capacity( struct axk_vector_t* in_handle )
{
    return in_handle == NULL ? 0UL : in_handle->elem_capacity;
}


void* axk_vector_buffer( struct axk_vector_t* in_handle )
{
    return in_handle == NULL ? NULL : in_handle->buffer;
}


void* axk_vector_insert( struct axk_vector_t* in_handle, uint64_t index, void* in_elem )
{
    return axk_vector_insert_range( in_handle, index, in_elem, 1UL );
}


void* axk_vector_insert_range( struct axk_vector_t* in_handle, uint64_t index, void* in_elem, uint64_t elem_count )
{
    if( in_handle == NULL || index > in_handle->elem_count || in_elem == NULL || elem_count == 0UL ) { return NULL; }

    // Check if we need to increase the size of the buffer
    if( in_handle->elem_count >= in_handle->elem_capacity )
    {
        expand_capacity( in_handle, elem_count );
    }

    // First, check for insertion to the back
    void* ins_begin;

    if( index == in_handle->elem_count )
    {
        // Were going to insert at the back of the buffer
        ins_begin = (void*)( (uint8_t*)( in_handle->buffer ) + ( in_handle->elem_size * in_handle->elem_count ) );
    }
    else
    {
        // Shift elements up, starting from the target index
        void* copy_start        = (void*)( (uint8_t*)( in_handle->buffer ) + ( in_handle->elem_size * index ) );
        void* copy_dest         = (void*)( (uint8_t*)( copy_start ) + ( in_handle->elem_size * elem_count ) );
        uint64_t copy_length    = in_handle->elem_size * ( in_handle->elem_count - index );

        // Use memmove to ensure we dont get corruped since the two buffer ranges overlap
        memmove( copy_dest, copy_start, copy_length );
        ins_begin = copy_start;
    }

    // Copy the elements in, either as a single block of memory if 'fn_copy' wasnt provided, or call 'fn_copy' on each element if it was
    if( in_handle->fn_copy == NULL )
    {
        memcpy( ins_begin, in_elem, elem_count * in_handle->elem_size );
    }
    else
    {
        for( uint64_t i = 0; i < elem_count; i++ )
        {
            in_handle->fn_copy( (void*)( (uint8_t*)( ins_begin ) + ( i * in_handle->elem_size ) ), (void*)( (uint8_t*)( in_elem ) + ( i * in_handle->elem_size ) ) );
        }
    }

    // Update the element counter and return the address of the first inserted element
    in_handle->elem_count += elem_count;
    return ins_begin;
}


bool axk_vector_erase( struct axk_vector_t* in_handle, uint64_t index )
{
    return axk_vector_erase_range( in_handle, index, 1UL );
}


bool axk_vector_erase_range( struct axk_vector_t* in_handle, uint64_t index, uint64_t count )
{
    if( in_handle == NULL || count == 0UL || ( index + count ) > in_handle->elem_count ) { return false; }

    // First, lets delete the elements that are no longer needed
    void* range_begin = (void*)( (uint8_t*)( in_handle->buffer ) + ( in_handle->elem_size * index ) );
    if( in_handle->fn_finalize == NULL )
    {
        memset( range_begin, 0, in_handle->elem_size * count );
    }
    else
    {
        for( uint64_t i = 0; i < count; i++ )
        {
            in_handle->fn_finalize( (void*)( (uint8_t*)( range_begin ) + ( i * in_handle->elem_size ) ) );
        }
    }

    // Next, lets check if we need to shift elements down to fill the empty space
    if( index != ( in_handle->elem_count - count ) )
    {
        void* shift_dest = (void*)( (uint8_t*)( in_handle->buffer ) + ( index * in_handle->elem_size ) );
        memmove( shift_dest, (void*)( (uint8_t*)( shift_dest ) + ( count * in_handle->elem_size ) ), ( in_handle->elem_count - ( index + count ) ) * in_handle->elem_size );
    }

    // Update the active element counter
    in_handle->elem_count -= count;
    return true;
}


void axk_vector_resize( struct axk_vector_t* in_handle, uint64_t new_size )
{
    // Check for null handles
    if( in_handle == NULL ) { return; }

    if( new_size == 0UL )
    {
        axk_vector_clear( in_handle );
    }
    else if( new_size < in_handle->elem_count )
    {
        // We have to shrink the number of active elements, so we need to invoke the finalization function if provided
        if( in_handle->fn_finalize == NULL )
        {
            void* updated_end = (void*)( (uint8_t*)( in_handle->buffer ) + ( new_size * in_handle->elem_size ) );
            memset( updated_end, 0, ( in_handle->elem_count - new_size ) * in_handle->elem_size );
        }
        else
        {
            for( uint64_t i = new_size; i < in_handle->elem_count; i++ )
            {
                in_handle->fn_finalize( (void*)( (uint8_t*)( in_handle->buffer ) + ( i * in_handle->elem_size ) ) );
            }
        }

        // Update the active element counter
        in_handle->elem_count = new_size;
    }
    else if( new_size > in_handle->elem_count )
    {
        // Now we have to grow the vector, were going to zero-fill the extra space 
        uint64_t diff = new_size - in_handle->elem_count;
        expand_capacity( in_handle, diff );
        in_handle->elem_count = new_size;
    }   
}


void axk_vector_shrink( struct axk_vector_t* in_handle )
{
    // Check for null handles
    if( in_handle == NULL ) { return; }

    // If the current capacity is at the minimum dont even check
    if( in_handle->elem_capacity <= MIN_CAPACITY || in_handle->buffer == NULL ) { return; }

    // Calculate what the capacity 'should' be for the active element count, and if its different than the actual capacity, resize buffer
    uint64_t new_capacity = calculate_capacity( in_handle, in_handle->elem_count );
    if( new_capacity + new_capacity <= in_handle->elem_capacity )
    {
        void* new_buffer = calloc( new_capacity, in_handle->elem_size );
        memcpy( new_buffer, in_handle->buffer, in_handle->elem_count * in_handle->elem_size );
        free( in_handle->buffer );

        in_handle->buffer          = new_buffer;
        in_handle->elem_capacity   = new_capacity;
    }
}


void axk_vector_reserve( struct axk_vector_t* in_handle, uint64_t new_capacity )
{
    // Check for null handles
    if( in_handle == NULL ) { return; }

    // Check if we need to increase the internal buffer size
    if( new_capacity > in_handle->elem_capacity )
    {
        expand_capacity( in_handle, new_capacity - in_handle->elem_capacity );
    }
}


void* axk_vector_push_back( struct axk_vector_t* in_handle, void* in_elem )
{
    return axk_vector_insert_range( in_handle, in_handle->elem_count, in_elem, 1UL );
}


void* axk_vector_push_front( struct axk_vector_t* in_handle, void* in_elem )
{
    return axk_vector_insert_range( in_handle, 0UL, in_elem, 1UL );
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


bool axk_vector_compare( struct axk_vector_t* lhs, struct axk_vector_t* rhs, int( *fn_compare )( void*, void* ) )
{
    if( lhs == rhs )    { return 0; }
    if( lhs == NULL )   { return 1; }   // && rhs != NULL
    if( rhs == NULL )   { return -1; }  // && lhs != NULL

    if( fn_compare == NULL )
    {
        // Compare the active element portion of the two buffers contained within the vectors
        // We are going to clamp the length to the size of the smallest active element count vector
        int cmp_result = memcmp( lhs->buffer, rhs->buffer, lhs->elem_count > rhs->elem_count ? lhs->elem_count * lhs->elem_size : rhs->elem_count * rhs->elem_size );
        if( cmp_result != 0 ) { return cmp_result; }
    }
    else
    {
        uint64_t cmp_count = lhs->elem_count > rhs->elem_count ? lhs->elem_count : rhs->elem_count; 
        for( uint64_t i = 0; i < cmp_count; i++ )
        {
            int cmp_result = fn_compare( 
                (void*)( (uint8_t*)( lhs->buffer ) + ( i * lhs->elem_size ) ),
                (void*)( (uint8_t*)( rhs->buffer ) + ( i * rhs->elem_size ) ) );
            
            if( cmp_result != 0 ) { return cmp_result; }
        }
    }

    // If all active elements we were able to compare were the same, lets check if one of the vectors has more elements than the other
    if( lhs->elem_count == rhs->elem_count ) { return 0; }
    return( lhs->elem_count > rhs->elem_count ? 1 : -1 );
}


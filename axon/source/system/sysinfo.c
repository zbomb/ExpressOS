/*==============================================================
    Axon Kernel - System Info
    2021, Zachary Berry
    axon/source/system/sysinfo.c
==============================================================*/

#include "axon/system/sysinfo_private.h"
#include "axon/system/sysinfo.h"
//#include "axon/library/rbtree.h"
#include "axon/library/spinlock.h"
#include "axon/library/atomic.h"
#include "stdlib.h"
#include "string.h"

/*
    Sysinfo Frame Structure
*/
struct axk_sysinfo_frame_t
{
    uint64_t size;
    // Data is placed after the end of the structure
};

/*
    State
*/
static struct axk_spinlock_t g_container_lock;
//static struct axk_rbtree_t g_container;
static struct axk_atomic_uint64_t g_counters[ AXK_COUNTER_MAX_INDEX + 1 ];

/*
    Initialize System Info State
*/
/*
bool axk_sysinfo_init( void )
{
    // Initialize state
    AXK_ZERO_MEM( g_container );
    axk_rbtree_create( &g_container, sizeof( void* ), NULL, NULL );
    axk_spinlock_init( &g_container_lock );

    return true; // TODO: Dont really need to return a boolean yet
}
*/


void axk_counters_init( void )
{
    for( uint32_t i = 0; i <= AXK_COUNTER_MAX_INDEX; i++ )
    {
        axk_atomic_store_uint64( g_counters + i, 0UL, MEMORY_ORDER_SEQ_CST );
    }
}

/*
    Write System Info
*/
/*
void axk_sysinfo_write( uint32_t index, uint32_t sub_index, void* ptr_data, uint64_t data_size )
{
    // Allocate new sysinfo frame and copy the data into it
    struct axk_sysinfo_frame_t* ptr_frame = NULL;
    if( ptr_data != NULL && data_size > 0UL )
    {
        ptr_frame           = (struct axk_sysinfo_frame_t*) malloc( sizeof( struct axk_sysinfo_frame_t ) + data_size );
        ptr_frame->size     = data_size;

        memcpy( (void*)( (uint8_t*)( ptr_frame ) + sizeof( struct axk_sysinfo_frame_t ) ), ptr_data, data_size );
    }

    // Acquire lock on the data container
    axk_spinlock_acquire( &g_container_lock );
    axk_rbtree_insert_or_update( &g_container, ( (uint64_t)( index ) << 32 ) | (uint64_t)( sub_index ), (void*) &ptr_frame );
    axk_spinlock_release( &g_container_lock );
}
*/
/*
    Query System Info
*/
/*
bool axk_sysinfo_query( uint32_t index, uint32_t sub_index, void* out_data, uint64_t data_size )
{
    // Check the system info data structure for the index/sub_index combination
    void* data_pointer = axk_rbtree_search_fast( &g_container, ( (uint64_t)( index ) << 32 ) | (uint64_t)( sub_index ) );
    if( data_pointer == NULL ) { return false; }

    // Check to ensure the data size matches
    struct axk_sysinfo_frame_t* ptr_frame = *((struct axk_sysinfo_frame_t**)( data_pointer ));
    if( ptr_frame == NULL ) { return false; }

    if( ptr_frame->size != data_size || ptr_frame->size == 0UL ) { return false; }

    // Copy the data to the output buffer if desired
    if( out_data != NULL )
    {
        memcpy( out_data, (void*)( (uint8_t*)( ptr_frame ) + sizeof( struct axk_sysinfo_frame_t ) ), ptr_frame->size );
    }

    return true;
}
*/
/*
    System Counter Functions
*/
uint64_t axk_counter_increment( uint32_t index, uint64_t diff )
{
    if( index > AXK_COUNTER_MAX_INDEX ) { return 0UL; }
    return( axk_atomic_fetch_add_uint64( g_counters + index, diff, MEMORY_ORDER_SEQ_CST ) + diff );
}


uint64_t axk_counter_decrement( uint32_t index, uint64_t diff )
{
    if( index > AXK_COUNTER_MAX_INDEX ) { return 0UL; }
    return( axk_atomic_fetch_sub_uint64( g_counters + index, diff, MEMORY_ORDER_SEQ_CST ) - diff );
}


uint64_t axk_counter_write( uint32_t index, uint64_t value )
{
    if( index > AXK_COUNTER_MAX_INDEX ) { return 0UL; }
    axk_atomic_store_uint64( g_counters + index, value, MEMORY_ORDER_SEQ_CST );
    return value;
}


uint64_t axk_counter_read( uint32_t index )
{
    if( index > AXK_COUNTER_MAX_INDEX ) { return 0UL; }
    return axk_atomic_load_uint64( g_counters + index, MEMORY_ORDER_SEQ_CST );
}
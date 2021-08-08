/*==============================================================
    Axon Kernel - Interlink
    2021, Zachary Berry
    axon/source/system/interlink.c
==============================================================*/

#include "axon/system/interlink.h"
#include "axon/system/interrupts_private.h"
#include "axon/library/rbtree.h"
#include "axon/system/sysinfo.h"
#include "axon/library/vector.h"
#include "axon/library/spinlock.h"
#include "axon/boot/basic_out.h"
#include "axon/arch.h"
#include "stdlib.h"
#include "string.h"

/*
    State
*/
struct axk_interlink_queue_t
{
    struct axk_vector_t messages;
    struct axk_spinlock_t lock;
};

static uint32_t g_handlers_count                = 0;
static struct axk_rbtree_t* g_handlers          = NULL;
static struct axk_interlink_queue_t* g_queues   = NULL;

/*
    Function Implementations
*/
bool axk_interlink_init( void )
{
    if( g_handlers != NULL ) { return false; }

    struct axk_sysinfo_general_t general_info;
    if( !axk_sysinfo_query( AXK_SYSINFO_GENERAL, 0, &general_info, sizeof( general_info ) ) ) { return false; }

    // Create the handler lists
    g_handlers_count = general_info.cpu_count;
    g_handlers = (struct axk_rbtree_t*) calloc( general_info.cpu_count, sizeof( struct axk_rbtree_t ) );
    if( g_handlers == NULL ) { return false; }

    for( uint32_t i = 0; i < g_handlers_count; i++ )
    {
        AXK_ZERO_MEM( g_handlers[ i ] );
        axk_rbtree_create( &( g_handlers[ i ] ), sizeof( void* ), NULL, NULL );
    }

    // Create message queues for each processor
    g_queues = (struct axk_interlink_queue_t*) calloc( g_handlers_count, sizeof( struct axk_interlink_queue_t ) );

    for( uint32_t i = 0; i < g_handlers_count; i++ )
    {
        AXK_ZERO_MEM( g_queues[ i ].messages );
        axk_vector_create( &( g_queues[ i ].messages ), sizeof( void* ), NULL, NULL );
        axk_spinlock_init( &( g_queues[ i ].lock ) );
    }

    return true;
}


void delete_message( struct axk_interlink_message_t* ptr_message )
{
    if( ptr_message == NULL ) { return; }
    if( !AXK_CHECK_FLAG( ptr_message->flags, AXK_INTERLINK_FLAG_DONT_FREE ) && ptr_message->body != NULL )
    {
        free( ptr_message->body );
        ptr_message->body = NULL;
    }

    free( ptr_message );
}


uint32_t axk_interlink_send( uint32_t target_cpu, struct axk_interlink_message_t* in_message, bool b_checked )
{
    // Ensure target CPU is valid and the message is present
    if( target_cpu >= g_handlers_count ) { return AXK_INTERLINK_ERROR_INVALID_TARGET; }
    if( in_message == NULL ) { return AXK_INTERLINK_ERROR_INVALID_MESSAGE; }

    // Copy the message, so the caller doesnt have to manually allocate or have a pointer to the internal message
    struct axk_interlink_message_t* new_message = (struct axk_interlink_message_t*) malloc( sizeof( struct axk_interlink_message_t ) );
    memcpy( new_message, in_message, sizeof( struct axk_interlink_message_t ) );
    axk_atomic_store_uint32( &( new_message->data_counter ), 1U, MEMORY_ORDER_RELAXED );
    new_message->source_cpu = axk_get_cpu_id();

    // Next, we need to acquire a lock on the target processors message queue
    struct axk_interlink_queue_t* target_queue = ( g_queues + target_cpu );

    // Now, insert the new message into the queue
    axk_spinlock_acquire( &( target_queue->lock ) );
    axk_vector_push_back( &( target_queue->messages ), (void*) new_message );
    axk_spinlock_release( &( target_queue->lock ) );

    // And finally, we need to send an IPI to let this processor know theres an additional message to process
    // TODO: Could we use atomics, so if the processor is already processing an interlink message, we dont send another IPI?
    struct axk_interprocessor_interrupt_t ipi;

    ipi.target_processor    = target_cpu;
    ipi.interrupt_vector    = AXK_INT_INTERLINK;
    ipi.delivery_mode       = AXK_IPI_DELIVERY_MODE_NORMAL;
    ipi.b_deassert          = false;
    ipi.b_wait_for_receipt  = b_checked;

    if( !axk_interrupts_send_ipi( &ipi ) )
    {
        // Scan through to see if the message is still in the queue, and delete it
        axk_spinlock_acquire( &( target_queue->lock ) );

        uint64_t count  = axk_vector_count( &( target_queue->messages ) );
        bool b_found    = false;

        for( uint64_t i = 0; i < count; i++ )
        {
            struct axk_interlink_message_t* m = (struct axk_interlink_message_t*) axk_vector_index( &( target_queue->messages ), i );
            if( m == new_message )
            {
                // Remove the message from the queue, and delete the message itself
                axk_vector_erase( &( target_queue->messages ), i );
                delete_message( new_message );
                b_found = true;
            }
        }

        axk_spinlock_release( &( target_queue->lock ) );

        // If we werent able to delete the message, that means it was already consumed, and we dont need to return an error
        if( b_found )
        {
            return AXK_INTERLINK_ERROR_DIDNT_SEND;
        }
    }

    return AXK_INTERLINK_ERROR_NONE;
}


uint32_t axk_interlink_broadcast( struct axk_interlink_message_t* in_message, bool b_include_self, bool b_checked )
{
    // Ensure message is valid
    if( in_message == NULL ) { return AXK_INTERLINK_ERROR_INVALID_MESSAGE; }

    // Copy the message, so the caller doesnt have to manually allocate or have a pointer to the internal message
    // Also, were going to use a counter, and whatever processor decrements it to zero is responsible for deleting the message
    uint32_t target_count = b_include_self ? g_handlers_count : g_handlers_count - 1U;
    struct axk_interlink_message_t* new_message = (struct axk_interlink_message_t*) malloc( sizeof( struct axk_interlink_message_t ) );
    memcpy( new_message, in_message, sizeof( struct axk_interlink_message_t ) );
    axk_atomic_store_uint32( &( new_message->data_counter ), target_count, MEMORY_ORDER_SEQ_CST );
    new_message->source_cpu = axk_get_cpu_id();

    // Loop through and send the message to each processor we are targetting
    bool b_failed = false;

    for( uint32_t i = 0; i < g_handlers_count; i++ )
    {
        if( !b_include_self && i == axk_get_cpu_id() ) { continue; }

        // Insert message into target processors queue
        struct axk_interlink_queue_t* target_queue = ( g_queues + i );

        axk_spinlock_acquire( &( target_queue->lock ) );
        axk_vector_push_back( &( target_queue->messages ), &new_message );
        axk_spinlock_release( &( target_queue->lock ) );

        // Attempt to send the IPI
        struct axk_interprocessor_interrupt_t ipi;

        ipi.target_processor    = i;
        ipi.interrupt_vector    = AXK_INT_INTERLINK;
        ipi.delivery_mode       = AXK_IPI_DELIVERY_MODE_NORMAL;
        ipi.b_deassert          = false;
        ipi.b_wait_for_receipt  = b_checked;

        if( !axk_interrupts_send_ipi( &ipi ) )
        {
            // Were not going to attempt to remove the message from the queue
            b_failed = true;
        }
    }

    return b_failed ? AXK_INTERLINK_ERROR_DIDNT_SEND : AXK_INTERLINK_ERROR_NONE;
}


void axk_interlink_set_handler( uint32_t type, void( *callback )( struct axk_interlink_message_t* ) )
{
    // Were going to use the message queue lock to ensure there are no conflicts with updating handlers
    uint32_t cpu_id = axk_get_cpu_id();
    struct axk_interlink_queue_t* this_queue = g_queues + cpu_id;
    axk_spinlock_acquire( &( this_queue->lock ) );

    if( callback == NULL )
    {
        axk_rbtree_erase_key( &( g_handlers[ cpu_id ] ), (uint64_t) type );
    }
    else
    {
        void* ptr_callback = (void*)( callback );
        axk_rbtree_insert_or_update( &( g_handlers[ cpu_id ] ), (uint64_t) type, (void*) &ptr_callback );
    }

    axk_spinlock_release( &( this_queue->lock ) );
}


void axk_interlink_handle_interrupt( void )
{
    uint32_t cpu_id = axk_get_cpu_id();

    // Loop through any messages in the queue and dispatch the handlers 
    // First though, we need to acquire a message queue lock
    struct axk_interlink_queue_t* this_queue = g_queues + cpu_id;
    axk_spinlock_acquire( &( this_queue->lock ) );

    while( axk_vector_count( &( this_queue->messages ) ) > 0UL )
    {
        struct axk_interlink_message_t* next_message = *( (struct axk_interlink_message_t**)axk_vector_get_front( &( this_queue->messages ) ) );
        if( next_message != NULL )
        {
            // Get the handler for this message
            void( *callback )( struct axk_interlink_message_t* );
            void* pp_callback = axk_rbtree_search_fast( g_handlers + cpu_id, (uint64_t) next_message->type );
            if( pp_callback != NULL )
            {
                callback = *( (void(**)(struct axk_interlink_message_t*))( pp_callback ) );
                if( callback != NULL )
                {
                    callback( next_message );
                }
            }

            // And finally, decrement the data counter to see if were responisble for freeing the message structure
            uint32_t other_owners = axk_atomic_fetch_sub_uint32( &( next_message->data_counter ), 1U, MEMORY_ORDER_SEQ_CST ) - 1U;
            if( other_owners == 0U )
            {
                delete_message( next_message );
            }
        }

        // Pop the processed message from the list
        axk_vector_pop_front( &( this_queue->messages ) );
    }

    axk_spinlock_release( &( this_queue->lock ) );
    axk_interrupts_signal_eoi();
}



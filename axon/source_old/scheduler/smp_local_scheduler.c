/*==============================================================
    Axon Kernel - SMP Local Scheduler (Thread Scheduler)
    2021, Zachary Berry
    axon/source/scheduler/smp_local_scheduler.c
==============================================================*/

#include "axon/scheduler/local_scheduler.h"
#include "axon/scheduler/global_scheduler.h"
#include "axon/arch.h"
#include "axon/library/rbtree.h"
#include "axon/boot/basic_out.h"
#include "axon/library/spinlock.h"
#include "axon/panic.h"
#include "string.h"
#include "stdlib.h"

/*
    Constants
*/
#define SOFT_REALTIME_LEVELS 5
#define HIGH_PRIORITY_LEVELS 5

/*
    axk_smp_thread_queue_t (STRUCT)
    * Private Structure
    * Holds a list of threads 
*/
struct axk_smp_thread_queue_t
{
    struct axk_thread_t* first;
    struct axk_thread_t* last;
    uint64_t count;
};


/*
    axk_smp_local_scheduler_t (STRUCT)
    * Private Structure
    * The 'normal' local scheduler instance used on SMP systems where there are no special requirments
*/
struct axk_smp_local_scheduler_t
{
    struct axk_local_scheduler_t func_table;

    bool b_init;
    uint32_t processor;

    struct axk_smp_thread_queue_t soft_realtime_queues[ SOFT_REALTIME_LEVELS ];
    struct axk_smp_thread_queue_t high_priority_queues[ HIGH_PRIORITY_LEVELS ];
    struct axk_rbtree_t normal_priority_tree;

    struct axk_spinlock_t soft_realtime_lock;
    struct axk_spinlock_t high_priority_lock;
    struct axk_spinlock_t normal_priority_lock;

};

/*
    Forward Decl.
*/
bool smpschd_init( struct axk_local_scheduler_t* );
bool smpschd_insert_thread( struct axk_local_scheduler_t*, struct axk_thread_t* );
bool smpschd_remove_thread( struct axk_local_scheduler_t*, struct axk_thread_t* );

/* 
    Function Impl
*/
struct axk_local_scheduler_t* axk_create_smp_scheduler( void )
{
    // Create a new instance of the scheduler
    struct axk_smp_local_scheduler_t* schd = (struct axk_smp_local_scheduler_t*) malloc( sizeof( struct axk_smp_local_scheduler_t ) );
    memset( schd, 0, sizeof( struct axk_smp_local_scheduler_t ) );

    // Load the function table
    schd->func_table.init = smpschd_init;

    // Set initial state
    schd->b_init        = false;
    schd->processor     = 0U;
    
    for( uint32_t i = 0; i < SOFT_REALTIME_LEVELS; i++ ) 
    { 
        schd->soft_realtime_queues[ i ].first   = NULL; 
        schd->soft_realtime_queues[ i ].last    = NULL;
        schd->soft_realtime_queues[ i ].count   = 0UL;
    }

    for( uint32_t i = 0; i < HIGH_PRIORITY_LEVELS; i++ ) 
    { 
        schd->high_priority_queues[ i ].first   = NULL; 
        schd->high_priority_queues[ i ].last    = NULL;
        schd->high_priority_queues[ i ].count   = 0UL;
    }

    return (struct axk_local_scheduler_t*)( schd );
}  


bool smpschd_init( struct axk_local_scheduler_t* self )
{
    struct axk_smp_local_scheduler_t* this = (struct axk_smp_local_scheduler_t*)( self );
    if( this == NULL || this->b_init ) { return false; }

    this->b_init        = true;
    this->processor     = axk_get_cpu_id();

    AXK_ZERO_MEM( this->normal_priority_tree );
    axk_rbtree_create( &( this->normal_priority_tree ), sizeof( void* ), NULL, NULL );
    axk_spinlock_init( &( this->soft_realtime_lock ) );
    axk_spinlock_init( &( this->high_priority_lock ) );
    axk_spinlock_init( &( this->normal_priority_lock ) );

    return true;
}


bool fifo_queue_insert( struct axk_smp_thread_queue_t* queue, struct axk_thread_t* thread )
{
    if( queue == NULL || thread == NULL ) { return false; }

    if( queue->last == NULL )
    {
        // Validate
        if( queue->first != NULL || queue->count != 0UL ) { return false; }

        queue->last     = thread;
        queue->first    = thread;
        queue->count    = 1UL;

        thread->next_thread = NULL;
    }
    else
    {
        struct axk_thread_t* old_tail = queue->last;

        queue->last                 = thread;
        queue->last->next_thread    = NULL;
        old_tail->next_thread       = queue->last;
        queue->count++;
    }
}


bool smpschd_insert_thread( struct axk_local_scheduler_t* self, struct axk_thread_t* thread )
{
    struct axk_smp_local_scheduler_t* this = (struct axk_smp_local_scheduler_t*)( self );
    if( this == NULL || thread == NULL ) { return false; }

    // Determine what queue to insert the thread into
    uint32_t calling_processor = axk_get_cpu_id();
    if( thread->policy == AXK_SCHEDULE_POLICY_NORMAL || thread->policy == AXK_SCHEDULE_POLICY_HIGH_PRIORITY )
    {
        // Insert into the high priority queue
        axk_spinlock_acquire( &( this->high_priority_lock ) );
        bool b_failed = fifo_queue_insert( &( this->high_priority_queues[ 0 ] ), thread );
        axk_spinlock_release( &( this->high_priority_lock ) );

        if( b_failed )
        {
            axk_panic( "Local Scheduler: corruption detected in local scheduler state!" );
        }
    }
    else if( thread->policy == AXK_SCHEDULE_POLICY_SOFT_REALTIME )
    {
        // Insert into the soft realtime queue
        axk_spinlock_acquire( &( this->soft_realtime_lock ) );
        bool b_failed = fifo_queue_insert( &( this->soft_realtime_queues[ 0 ] ), thread );
        axk_spinlock_release( &( this->soft_realtime_lock ) );

        if( b_failed )
        {
            axk_panic( "Local Scheduler: corruption detected in local scheduler state!" );
        }
    }
    else if( thread->policy == AXK_SCHEDULE_POLICY_BACKGROUND )
    {
        // Insert into normal queue
        axk_spinlock_acquire( &( this->normal_priority_lock ) );
        // TODO: Determine what key to use to insert into the normal priority list
        // We need each thread to have a unique key
        // Might require some changes to the rbtree algorithm to ensure there are no entries with the same key
        axk_spinlock_release( &( this->normal_priority_lock ) );
    }
    else
    {
        // Invalid policy!
        return false;
    }


}


bool smpschd_remove_thread( struct axk_local_scheduler_t* self, struct axk_thread_t* thread )
{
    return false;
}

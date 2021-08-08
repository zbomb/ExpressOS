/*==============================================================
    Axon Kernel - Thread Scheduler
    2021, Zachary Berry
    axon/private/axon/scheduler/thread_scheduler.h
==============================================================*/

#pragma once
#include "axon/config.h"
#include "axon/scheduler/global_scheduler.h"


/*
    What functions do we need?

    add_thread( ... );
    remove_thread( ... );

    These functions would be able to be called from any processor on the system though
    Unless, we funnel calls to these functions through an IPI, but.. IPIs cant contain data
    nativley, we would have to create an additional abstraction above the current system to pass data
    probably using some simple queue type system.. or something similar

    
    bool insert_thread( struct axk_local_scheduler_t* this, struct axk_thread_t* in_thread );
    bool remove_thread( struct axk_local_scheduler_t* this, struct axk_thread_t* in_thread );
*/


/*
    axk_local_scheduler_t (STRUCT)
    * Private Structure
    * Contains the function list for a local scheduler instance
*/
struct axk_local_scheduler_t
{
    bool( *init )( struct axk_local_scheduler_t* );
    bool( *insert_thread )( struct axk_local_scheduler_t*, struct axk_thread_t* );
    bool( *remove_thread )( struct axk_local_scheduler_t*, struct axk_thread_t* );
};

/*
    axk_create_smp_local_scheduler
    * Private Function
    * Creates an instance of the local SMP scheduler
*/
struct axk_local_scheduler_t* axk_create_smp_scheduler( void );


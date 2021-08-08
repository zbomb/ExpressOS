/*==============================================================
    Axon Kernel - Global Scheduler (Process Scheduler)
    2021, Zachary Berry
    axon/private/axon/scheduler/global_scheduler.h
==============================================================*/

#pragma once
#include "axon/config.h"
#include "axon/library/rbtree.h"
#include "axon/system/time.h"

enum axk_schedule_policy_t
{
    AXK_SCHEDULE_POLICY_NORMAL          = 0,
    AXK_SCHEDULE_POLICY_HIGH_PRIORITY   = 1,
    AXK_SCHEDULE_POLICY_SOFT_REALTIME   = 2,
    AXK_SCHEDULE_POLICY_BACKGROUND      = 3
};

enum axk_schedule_group_t
{
    AXK_SCHEDULE_GROUP_NORMAL                   = 0,
    AXK_SCHEDULE_GROUP_HIGH_PRIORITY_LEVEL_0      = 1,
    AXK_SCHEDULE_GROUP_HIGH_PRIORITY_LEVEL_1      = 2,
    AXK_SCHEDULE_GROUP_HIGH_PRIORITY_LEVEL_2      = 3,
    AXK_SCHEDULE_GROUP_HIGH_PRIORITY_LEVEL_3      = 4,
    AXK_SCHEDULE_GROUP_HIGH_PRIORITY_LEVEL_4      = 5,
    AXK_SCHEDULE_GROUP_HIGH_PRIORITY_LEVEL_5      = 6,
    AXK_SCHEDULE_GROUP_SOFT_REALTIME_LEVEL_0    = 7,
    AXK_SCHEDULE_GROUP_SOFT_REALTIME_LEVEL_1    = 8,
    AXK_SCHEDULE_GROUP_SOFT_REALTIME_LEVEL_2    = 9,
    AXK_SCHEDULE_GROUP_SOFT_REALTIME_LEVEL_3    = 10,
    AXK_SCHEDULE_GROUP_SOFT_REALTIME_LEVEL_4    = 11,
    AXK_SCHEDULE_GROUP_SOFT_REALTIME_LEVEL_5    = 12
};

enum axk_priority_t
{
    AXK_PRIORITY_MINIMUM    = 0,
    AXK_PRIORITY_LOW        = 1,
    AXK_PRIORITY_NORMAL     = 2,
    AXK_PRIORITY_HIGH       = 3,
    AXK_PRIORITY_MAXIMUM    = 4
};

enum axk_process_type_t
{
    AXK_PROCESS_TYPE_KERNEL             = 0,
    AXK_PROCESS_TYPE_KERNEL_DRIVER      = 1,
    AXK_PROCESS_TYPE_USER_APPLICATION   = 2,
    AXK_PROCESS_TYPE_USER_DRIVER        = 3,
    AXK_PROCESS_TYPE_USER_BACKGROUND    = 4
};

/*
    We havent even really decided on a scheduling algorithm
    I think theres two viable options

        1. Using multiple deque/queues, each representing a priority level, some static number of levels
            We have certain ranges of priorities that are grouped (i.e. soft-realtime, kernel, user, background)
            Then, depending on what the thread does (i.e. blocking for i/o or being computationally heavy) we move it within the group
            the more a thread blocks for i/o, we increase its priority level to ensure i/o happens really really fast
        2. Using a single rbtree per CPU, each keeping track of the amount of time the thread has recieved compared to what a fair split
           would be, factoring in its priority, and the perecntage off from this value would be the key to ensrue we fairly schedule all threads
           

           Another problem is, say a time-slice passes.. do we have to re-jigger the whole list? Yes if we use a percentage based system
           How can we calculate the key without needing constant updates each time slice?

           The basic idea is, the key is the amount of time the thread has recieved, but, longer threads will inherently have higher values
           We could add some offset to the key that represents a 'fair amount' of time this thread would have recieved up to this point.
           Maybe minus a bit so its inserted into the front of the tree, we can be sure it will be executed right away

            We could keep a list of new threads, we execute first. 

            Then, when we insert into the main list, it gets the average key value.

            I would like to use a tree

            We could also do a combination of this tree system, and a few normal queues for more 'important' threads like the real time threads



            So.... we can do something like....


            Real-Time: Highest importance, deque, always executed before any lower priority threads, must be created by kernel
            Real-Time -1
            Real-Time -2
            Real-Time -3
            Real-Time -4
            Real-Time -5

            Low Latency Threads: High importance, can be created from userspace or kernel space
            Low Latency Threads - 1
            Low Latency Threads - 2
            Low Latency Threads - 3
            Low Latency Threads - 4
            Low Latency Threads - 5

            Normal Threads: Low to medium priority, rbtree, executed on a 'fair' basis once higher priority threads have been executed

            * We should be able to automatically prompte normal threads to low latency threads if required
            * Also, if low latency threads are really killing time, we can move it into the normal thread list
            * Any interrupt based threads can be placed into the low latency list as well
            
            * The algorithm wouldd start by checking for real time threads in order of priority, Once each priority level is empty we move down
            * Real-time threads cant be demoted
            * Also, other threads cant be prompted into real-time thread lists
            
            * We need to guard against resource starvation, say if a thread uses its whole time slice 'X" times, it gets automaticallly moved into either
                the lowest real-time list (if its a real-time thread), or down into the normal thread list (if low-latency thread)
            
            * We can keep a counter to track how often it eats the whole time-slice.
            * Then, we can see if a thread is using very little of its time slice quite often, prompte it up
            
            * We need to have basically a seperate list to store the pending threads, this way we dont sit here and block over them

*/


struct axk_thread_t
{
    uint64_t id; 
    uint32_t processor;
    enum axk_priority_t priority;
    enum axk_schedule_policy_t policy;
    enum axk_schedule_group_t group;
    struct axk_time_t create_time;
    uint64_t run_time;
    const char* name;
    void* user_stack;
    void* kernel_stack;
    void* address_space;
    struct axk_process_t* process;
    struct axk_thread_t* next_thread;

    // This is arch-specific, each arch has its own way of storing processor state
    void* arch_state;
};


struct axk_process_t
{
    uint32_t id;
    enum axk_process_type_t type;
    enum axk_priority_t priority;
    const char* name;
    struct axk_rbtree_t threads;
};


/*
    axk_scheduler_init_global
    * Private Funcion
    * Creates the global scheduler instance
*/
bool axk_scheduler_init_global( void );

/*
    axk_scheduler_init_local
    * Private Function
    * Creates local scheduler instance for the processor which calls this function
*/
bool axk_scheduler_init_local( void );

/*
    axk_scheduler_on_thread_insert_failed
    * Private Function
    * Called from the local scheduler when a thread wasnt able to be inserted for any reason
*/
void axk_scheduler_on_thread_insert_failed( struct axk_thread_t* thread, uint32_t processor );

/*

    What functions should we write?

    axk_processor_scheduler_init  (BSP ONLY)
    axk_thread_scheduler_init5

    axk_process_create( struct axk_create_process_parameters_t* in_params );
    axk_thread_create( struct axk_create_thread_parameters_t* in_params );

    * This is a little confusing, but we can create a process from anywhere
    * But, creating a thread is more complex, youre limited to what kind of threads you can create based on where it was called from 
    
    * 'thread_create' can only create threads from the process it was called from
    * How can we quickly get the current process on a processor? Do we have some register that we can use to store an identifier or something?
    
    * axk_thread_get_state( void ); ? Which contains the current process identifier and thread identifier





    * We want to allow a very high amount of customization in how scheduling works, and allow for AMP and SMP
    So... certain platforms might opt for specific scheduler settings
    There is one type of global scheduler, which is the public scheduler interface
    There can be multiple types of the local schedulers, either per-platform or even per-system based on a bunch


    




*/

/*==============================================================
    Axon Kernel - Event System (Private Header)
    2021, Zachary Berry
    axon/private/axon/system/events_private.h
==============================================================*/

#pragma once
#include "axon/system/events.h"


/*
    axk_event_shared_state_t (STRUCTURE)
    * Private Structure
    * Both the event instance, and an event token contain a pointer to this structure
    * Used to relay information from the event system back to a caller who wants to inquire about the status or cancel at will
*/
struct axk_event_shared_state_t
{
    struct axk_event_t* event;
    enum axk_event_status_t status;
    struct axk_atomic_flag_t cancel_flag;
};

/*
    axk_event_t (STRUCTURE)
    * Private Structure
    * Holds the state for a schedueld event
    * Created internally by the event system, no need for public use of this structure
*/
struct axk_event_t
{
    bool( *callback )( uint64_t );
    struct axk_time_t start_time;
    struct axk_time_t recur_period;
    enum axk_event_priority_t priority;
    uint64_t recur_count;
    struct axk_event_shared_state_t shared_state;
};

/*
    axk_events_init
    * Private Function
    * Initializes the event system
*/
bool axk_events_init( void );

/*
    axk_events_count
    * Private Function
    * Returns the total number of pending events in the event system
*/
uint64_t axk_events_count( void );


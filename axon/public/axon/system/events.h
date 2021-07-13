/*==============================================================
    Axon Kernel - Event System
    2021, Zachary Berry
    axon/public/axon/system/events.h
==============================================================*/

#pragma once
#include "axon/system/time.h"
#include "axon/memory/atomics.h"

/*
    Constants
*/
#define AXK_EVENT_COUNT_INFINITE 0xFFFFFFFFFFFFFFFFUL

/*
    axk_event_priority_t (ENUM)
    * Used to describe how 'important' it is to execute an event before others that have a very similar deadline
    * If multiple events are within a few nanoseconds of each other, they will be grouped and ran in order of this priority, high to low
*/
enum axk_event_priority_t
{
    AXK_EVENT_PRIORITY_MINIMUM  = 0,
    AXK_EVENT_PRIORITY_LOW      = 1,
    AXK_EVENT_PRIORITY_MEDIUM   = 2,
    AXK_EVENT_PRIORITY_HIGH     = 3,
    AXK_EVENT_PRIORITY_MAXIMUM  = 4
};

/*
    axk_event_status_t (ENUM)
    * Returned from 'axk_event_get_status', describing the current status obviously
*/
enum axk_event_status_t
{
    AXK_EVENT_STATUS_PENDING    = 0,
    AXK_EVENT_STATUS_COMPLETE   = 1,
    AXK_EVENT_STATUS_RECURRING  = 2,
    AXK_EVENT_STATUS_CANCELLED  = 3
};

/*
    axk_event_create_result_t (ENUM)
    * Returned from 'axk_event_create' and 'axk_event_create_recurring'
*/
enum axk_event_create_result_t
{
    AXK_EVENT_CREATE_SUCCESS            = 0,
    AXK_EVENT_CREATE_INVALID_TIME       = 1,
    AXK_EVENT_CREATE_INVALID_PARAMS     = 2
};

/*
    axk_event_cancel_result_t (ENUM)
    * Returned from 'axk_event_cancel'
*/
enum axk_event_cancel_result_t 
{
    AXK_EVENT_CANCEL_SUCCESS            = 0,
    AXK_EVENT_CANCEL_ALREADY_COMPLETE   = 1,
    AXK_EVENT_CANCEL_ERROR              = 2
};

/*
    axk_event_token_t (STRUCTURE)
    * Can be initialized using 'axk_event_token_init'
    * Once initialized, it MUST be destroyed using 'axk_event_token_destroy' to prevent memory leaks
    * Can be passed into a call to create an event, in which case the token can then be used to get the status of, or cancel a pending event
*/
struct axk_event_token_t
{
    struct axk_event_shared_state_t* shared_state;
};

/*
    axk_event_token_init
    * Initializes the state for an event token
    * Note: MUST be called on a token before you pass this token to 'axk_event_create(_recurring)'
    * Note: MUST call 'axk_event_token_destroy' on the same token after you are done using it
*/
void axk_event_token_init( struct axk_event_token_t* token );

/*
    axk_event_token_destroy
    * Destroy the state of an event token
    * MUST be called before an event token is freed/goes out of scope
*/
void axk_event_token_destroy( struct axk_event_token_t* token );

/*
    axk_event_create
    * Creates a normal, one-shot event that will occur at the specified time (or as close to it as possible)
    * Priority is used when there is multiple events to execute around the same time, theyre executed from high to low priority
    * Callback must be non-null
    * For one-shot mode, the parameter passed to the callback will always be '0', and the return will be ignored
    * You can create (axk_event_create_token) a token and pass it in to be able to track the state, or cancel the event 
    * NOTE: If you create a token (axk_event_create_token), you MUST destroy it when its no longer needed! (axk_event_destroy_token)
*/
enum axk_event_create_result_t axk_event_create( bool( *callback )( uint64_t ), struct axk_time_t target_time, enum axk_event_priority_t priority, struct axk_event_token_t* in_token );

/*
    axk_event_create_recurring
    * Creates a recurring event, with an initial delay, a period and an optional execute count
    * Priority is used when there is multiple events to execute around the same time, they are executed in order of high to low priority in this case
    * Callback must be non-null
    * For periodic mode, the parameter passed to the callback will be the remaining ticks after the current call
    * For periodic mode, the return value from the callback can be used to cancel the recurring event
    * You can create (axk_event_create_token) a token and pass it into this function, which will allow you to use this token to track the event or even cancel it
    * NOTE: If you create a token, you MUST destroy it once its no longer needed, otherwise a small memory leak will occur
*/
enum axk_event_create_result_t axk_event_create_recurring( bool( *callback )( uint64_t ), struct axk_time_t init_time, struct axk_time_t recur_time, uint64_t recur_count,
    enum axk_event_priority_t priority, struct axk_event_token_t* in_token );

/*
    axk_event_get_status
    * Gets the status of an event
    * 'token' must have been passed to a 'create event' function to hold a reference to an event, and also been properly initialized
*/
enum axk_event_status_t axk_event_get_status( struct axk_event_token_t* token );

/*
    axk_event_cancel
    * Attempts to cancel an event using an event token
    * The return value will indicate what happened, ex. the event was already cancelled
    * 'token' must have been passed to a 'create event' function to hold a reference to an event, and also been properly initialized
*/
enum axk_event_cancel_result_t axk_event_cancel( struct axk_event_token_t* token );

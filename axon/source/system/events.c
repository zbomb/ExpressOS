/*==============================================================
    Axon Kernel - Event System
    2021, Zachary Berry
    axon/source/system/events.c
==============================================================*/

#include "axon/system/events_private.h"
#include "stdlib.h"
#include "string.h"



bool axk_events_init( void )
{
    return true;
}


uint64_t axk_events_count( void )
{

}


enum axk_event_create_result_t axk_event_create( bool( *callback )( uint64_t ), struct axk_time_t target_time, enum axk_event_priority_t priority, struct axk_event_token_t* in_token )
{
    return AXK_EVENT_CREATE_SUCCESS;
}


enum axk_event_create_result_t axk_event_create_recurring( bool( *callback )( uint64_t ), struct axk_time_t init_time, struct axk_time_t recur_time, uint64_t recur_count,
    enum axk_event_priority_t priority, struct axk_event_token_t* in_token )
{
    return AXK_EVENT_CREATE_SUCCESS;
}


enum axk_event_status_t axk_event_get_status( struct axk_event_token_t* token )
{
    return AXK_EVENT_STATUS_PENDING;
}


enum axk_event_cancel_result_t axk_event_cancel( struct axk_event_token_t* token )
{
    return AXK_EVENT_CANCEL_SUCCESS;
}


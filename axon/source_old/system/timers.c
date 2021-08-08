/*==============================================================
    Axon Kernel - Timers
    2021, Zachary Berry
    axon/source/system/timers.c
==============================================================*/

#include "axon/system/timers.h"
#include "axon/system/interrupts.h"
#include "axon/boot/basic_out.h"
#include "axon/system/timers_private.h"

/*
    Function Implementations
*/
struct axk_timer_driver_t* axk_timer_get_by_id( uint32_t id )
{
    uint32_t count = axk_timer_get_count();
    for( uint32_t i = 0; i < count; i++ )
    {
        struct axk_timer_driver_t* ptr_timer = axk_timer_get( i );
        if( ptr_timer->get_id() == id ) { return ptr_timer; }
    }

    return NULL;
}


bool axk_timer_query_features( struct axk_timer_driver_t* timer, enum axk_timer_features_t feats )
{
    // Check parameters
    if( timer == NULL || AXK_CHECK_FLAG( feats, AXK_TIMER_FEATURE_NONE ) ) { return false; }
    return timer->query_features( timer, feats );
}


uint32_t axk_timer_get_id( struct axk_timer_driver_t* timer )
{
    if( timer == NULL ) { return AXK_TIMER_ID_NONE; }
    return timer->get_id();
}


uint64_t axk_timer_get_frequency( struct axk_timer_driver_t* timer )
{
    if( timer == NULL ) { return 0UL; }
    return timer->get_frequency( timer );
}


uint32_t axk_timer_start( struct axk_timer_driver_t* timer, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, uint32_t processor, uint8_t vector )
{
    if( timer == NULL ) { return AXK_TIMER_ERROR_INVALID_PARAMS; }
    return timer->start( timer, mode, delay, b_delay_in_ticks, processor, vector );
}


bool axk_timer_stop( struct axk_timer_driver_t* timer )
{
    if( timer == NULL ) { return false; }
    return timer->stop( timer );
}


bool axk_timer_is_running( struct axk_timer_driver_t* timer )
{
    if( timer == NULL ) { return false; }
    return timer->is_running( timer );
}


uint64_t axk_timer_get_counter_value( struct axk_timer_driver_t* timer )
{
    if( timer == NULL ) { return 0UL; }
    return timer->get_counter( timer );
}


uint64_t axk_timer_get_max_value( struct axk_timer_driver_t* timer )
{
    if( timer == NULL ) { return 0UL; }
    return timer->get_max_value( timer );
}

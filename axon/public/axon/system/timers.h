/*==============================================================
    Axon Kernel - Timer System
    2021, Zachary Berry
    axon/public/axon/system/timers.h
==============================================================*/

#pragma once
#include "axon/config.h"


enum axk_timer_features_t
{
    AXK_TIMER_FEATURE_NONE              = 0x00,
    AXK_TIMER_FEATURE_ONE_SHOT          = 0x01,
    AXK_TIMER_FEATURE_PERIODIC          = 0x02,
    AXK_TIMER_FEATURE_COUNTER           = 0x04,
    AXK_TIMER_FEATURE_INVARIANT         = 0x08,
    AXK_TIMER_FEATURE_DEADLINE          = 0x10,
    AXK_TIMER_FEATURE_LOCAL             = 0x20,
    AXK_TIMER_FEATURE_EXTERNAL          = 0x40,
    AXK_TIMER_FEATURE_DIVISOR           = 0x80
};


enum axk_timer_mode_t
{
    AXK_TIMER_MODE_ONE_SHOT     = 0x00,
    AXK_TIMER_MODE_PERIODIC     = 0x01,
    AXK_TIMER_MODE_DEADLINE     = 0x02,
    AXK_TIMER_MODE_DIVISOR      = 0x03
};

#define AXK_TIMER_ERROR_NONE                0x00
#define AXK_TIMER_ERROR_INVALID_MODE        0x01
#define AXK_TIMER_ERROR_ALREADY_RUNNING     0x02
#define AXK_TIMER_ERROR_INVALID_PARAMS      0x03


struct axk_timer_driver_t
{
    bool( *init )( struct axk_timer_driver_t* );
    bool( *query_features )( struct axk_timer_driver_t*, enum axk_timer_features_t );
    uint32_t( *get_id )( void );
    uint64_t( *get_frequency )( struct axk_timer_driver_t* );
    uint32_t( *start )( struct axk_timer_driver_t*, enum axk_timer_mode_t, uint64_t, bool, bool(*)(void) );
    bool( *stop )( struct axk_timer_driver_t* );
    bool( *is_running )( struct axk_timer_driver_t* );
    uint64_t( *get_counter )( struct axk_timer_driver_t* );
    uint64_t( *get_max_value )( struct axk_timer_driver_t* );
    bool( *invoke )( struct axk_timer_driver_t* );
};


/*
    axk_timer_get_count
    * Gets the number of timers available on the system
*/
uint32_t axk_timer_get_count( void );

/*
    axk_timer_get
    * Gets a timer at the specified index
    * Use 'axk_timer_get_count' to determine the total number of timers on the system
    * Index is 0-based, so if the count is 3, there are timers at indicies [0,1,2]
*/
struct axk_timer_driver_t* axk_timer_get( uint32_t index );

/*
    axK_timer_get_by_id
    * Gets a specific timer
*/
struct axk_timer_driver_t* axk_timer_get_by_id( uint32_t id );

/*
    axk_timer_get_local
    * Gets the timer selected to be used as the preferred system local timer
*/
struct axk_timer_driver_t* axk_timer_get_local( void );

/*
    axk_timer_get_external
    * Gets the timer selected to be used as the preferred system external timer
*/
struct axk_timer_driver_t* axk_timer_get_external( void );

/*
    axk_timer_get_counter
    * Gets the timer selected to be used as the preferred system counter
*/
struct axk_timer_driver_t* axk_timer_get_counter( void );

/*
    axk_delay
    * Pauses the current processor core for the specified number of nanoseconds
*/
void axk_delay( uint64_t in_nano );


/*
    axk_timer_query_features
    * Queries a timer for a list (or a single) of features
    * Returns true if ALL features specified are supported
*/
bool axk_timer_query_features( struct axk_timer_driver_t* timer, enum axk_timer_features_t feats );

/*
    axk_timer_get_id
    * Queries a timer for its unique identifier
*/
uint32_t axk_timer_get_id( struct axk_timer_driver_t* timer );

/*
    axk_timer_get_frequency
    * Gets the frequency at which the clock that the timer is based on, runs at, in Hz
*/
uint64_t axk_timer_get_frequency( struct axk_timer_driver_t* timer );

/*
    axk_timer_start
    * Starts a timer with the specified mode
    * Returns error code (AXK_TIMER_ERROR_*)
    * If successful, returns AXK_TIMER_ERROR_NONE
*/
uint32_t axk_timer_start( struct axk_timer_driver_t* timer, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, bool( *callback )( void ) );

/*
    axk_timer_stop
    * Stops a timer
    * Returns true if the timer could be stopped
*/
bool axk_timer_stop( struct axk_timer_driver_t* timer );

/*
    axk_timer_is_running
    * Returns true if a timer is currently running
*/
bool axk_timer_is_running( struct axk_timer_driver_t* timer );

/*
    axk_timer_get_counter
    * Gets the current counter value for this timer
    * Not all timers support this, check using AXK_TIMER_FEATURE_COUNTER
    * If not supported, returns 0
*/
uint64_t axk_timer_get_counter_value( struct axk_timer_driver_t* timer );

/*
    axk_timer_get_max_value
    * Gets the max counter value
    * For timers that support 'Divisor Mode', this would be the max value
    * When specifying the delay as 'ticks', this is the max value
*/
uint64_t axk_timer_get_max_value( struct axk_timer_driver_t* timer );








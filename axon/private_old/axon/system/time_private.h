/*==============================================================
    Axon Kernel - Private Time Keeping Functions
    2021, Zachary Berry
    axon/private/axon/system/time_private.h
==============================================================*/

#pragma once
#include "axon/config.h"
#include "axon/system/time.h"


/*
    axk_time_init
    * Private Function
    * Called from the timer system to setup the time keeping state
*/
void axk_time_init( uint64_t ext_tick_period );

/*
    axk_time_ext_tick
    * Private Function
    * Callback for the external timer used to keep system time
*/
//void axk_time_ext_tick( void );

/*
    axk_time_wait_for_sync
    * Private Function
    * After setting up the external timer to periodically call 'axk_time_ext_tick', this should be called
      to wait for the time system to stabilize and syncronize with the persistent hardware clock
*/
void axk_time_wait_for_sync( void );

/*
    axk_time_init_persistent_clock
    * Private Function
    * Implemented in architecture specific code
*/
bool axk_time_init_persistent_clock( void );

/*
    axk_time_read_persistent_clock
    * Private Function
    * Reads the hardware clock value
    * Implemented in architecture specific code
*/
bool axk_time_read_persistent_clock( struct axk_date_t* out_date, uint64_t* out_counter_sync );

/*
    axk_time_write_persistent_clock
    * Private Function
    * Writes a new time to the hardware clock
    * Implemented in architecture specific code
*/
bool axk_time_write_persistent_clock( struct axk_date_t* in_date );


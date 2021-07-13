/*==============================================================
    Axon Kernel - Private Timer System Functions
    2021, Zachary Berry
    axon/private/axon/system/timers_private.h
==============================================================*/

#pragma once
#include "axon/system/timers.h"

/*
    axk_timers_init
    * Private Function
    * Implemented in arch-specific code file
    * Detects installed timers and initializes the corresponding dirvers, selected preferred timer sources
*/
bool axk_timers_init( void );

/*
    axk_timers_bsp_sync
    * Private Function
    * Syncronizes the main system counter, with the hardware wall clock to get an accurate system time
    * Also syncronize with the various processors
    * Will wait for the APs to call 'axk_timers_aux_sync'
    * Implemented in arch-specific code 
*/
bool axk_timers_bsp_sync( void );

/*
    axk_timers_ap_sync
    * Private Function
    * Called from each AP during boot to syncronize counters
    * Implemented in arch-specific code
*/
bool axk_timers_ap_sync( void );


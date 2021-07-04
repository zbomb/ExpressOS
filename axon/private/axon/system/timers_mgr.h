/*==============================================================
    Axon Kernel - Timer Managment
    2021, Zachary Berry
    axon/private/axon/system/timers_mgr.h
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

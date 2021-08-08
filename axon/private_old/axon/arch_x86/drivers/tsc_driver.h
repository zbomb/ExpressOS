/*==============================================================
    Axon Kernel - X86 TSC Driver
    2021, Zachary Berry
    axon/private/axon/arch_x86/tsc_driver.h
==============================================================*/

#pragma once
#include "axon/system/timers.h"
#include "axon/memory/atomics.h"

/*
    TSC Driver State
*/
struct axk_x86_tsc_driver_t
{
    struct axk_timer_driver_t func_table;
    uint64_t frequency;
};


/*
    axk_x86_create_tsc_driver
    * Private Function (x86 only)
    * Creates an invariant TSC driver instance
*/
struct axk_timer_driver_t* axk_x86_create_tsc_driver( void );

/*
    axk_x86_calibrate_tsc_driver
    * Private Function (x86 only)
    * Calibrates the rate of the counter
    * Only needs to be called on BSP, since its invariant
    * To ensure all CPUs share the same rate, we perform a syncronization
*/
bool axk_x86_calibrate_tsc_driver( struct axk_timer_driver_t* self );
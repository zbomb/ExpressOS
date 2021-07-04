/*==============================================================
    Axon Kernel - X86 HPET Driver
    2021, Zachary Berry
    axon/private/axon/arch_x86/hpet_driver.h
==============================================================*/

#pragma once
#include "axon/system/timers.h"


/*
    HPET Driver State
*/
struct axk_x86_hpet_driver_t
{
    struct axk_timer_driver_t func_table;
    struct axk_x86_hpet_info_t* info;
};


/*
    axk_x86_create_hpet_driver
    * Private Function (x86 only)
    * Creates an HPET driver instance
*/
struct axk_timer_driver_t* axk_x86_create_hpet_driver( struct axk_x86_hpet_info_t* ptr_info );
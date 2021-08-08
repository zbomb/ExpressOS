/*==============================================================
    Axon Kernel - X86 PIT Driver
    2021, Zachary Berry
    axon/private/axon/arch_x86/pit_driver.h
==============================================================*/

#pragma once
#include "axon/system/timers.h"
#include "axon/library/spinlock.h"


/*
    PIT Driver State
*/
struct axk_x86_pit_driver_t
{
    struct axk_timer_driver_t func_table;

    uint32_t global_interrupt;
    uint32_t target_processor;
    uint8_t target_interrupt;
    struct axk_spinlock_t lock;
};


/*
    axk_x86_create_pit_driver
    * Private Function (x86 only)
    * Creates a PIT driver instance
*/
struct axk_timer_driver_t* axk_x86_create_pit_driver( void );
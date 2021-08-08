/*==============================================================
    Axon Kernel - X86 HPET Driver
    2021, Zachary Berry
    axon/private/axon/arch_x86/hpet_driver.h
==============================================================*/

#pragma once
#include "axon/system/timers.h"
#include "axon/library/spinlock.h"

/*
    HPET Driver State
*/
struct axk_x86_hpet_driver_t
{
    struct axk_timer_driver_t func_table;
    struct axk_x86_hpet_info_t* info;

    // Info about the whole HPET
    uint16_t min_tick;
    uint64_t base_address;
    uint64_t period;
    bool b_long_counter;

    // Info about the selected timer
    bool b_timer_long;
    bool b_timer_fsb;
    uint8_t timer_index;
    uint32_t global_interrupt;
    uint32_t target_processor;
    uint8_t target_interrupt;

    // State
    struct axk_spinlock_t lock;
};


/*
    axk_x86_create_hpet_driver
    * Private Function (x86 only)
    * Creates an HPET driver instance
*/
struct axk_timer_driver_t* axk_x86_create_hpet_driver( struct axk_x86_hpet_info_t* ptr_info );
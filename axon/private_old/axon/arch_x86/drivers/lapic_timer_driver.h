/*==============================================================
    Axon Kernel - X86 Local-APIC Timer Driver
    2021, Zachary Berry
    axon/private/axon/arch_x86/lapic_timer_driver.h
==============================================================*/

#pragma once
#include "axon/system/timers.h"


/*
    LAPIC Timer Driver State
*/
struct axk_x86_lapic_timer_driver_t
{
    struct axk_timer_driver_t func_table;

    bool b_init;
    bool b_deadline_mode;
    bool b_constant;
    
    struct axk_x86_xapic_driver_t* ptr_xapic;
    struct axk_x86_x2apic_driver_t* ptr_x2apic;

    uint64_t frequency;
};


/*
    axk_x86_create_lapic_timer_driver
    * Private Function (x86 only)
    * Creates a Local-APIC Timer Driver
*/
struct axk_timer_driver_t* axk_x86_create_lapic_timer_driver( void );

/*
    axk_x86_calibrate_lapic_timer
    * Private Function (x86 only)
    * Calibrates the local apic timer, on the current core
    * Only needs to be called on BSP
    * TODO: Can we pass TSC calibration value into here?
*/
bool axk_x86_calibrate_lapic_timer( struct axk_timer_driver_t* self );

/*==============================================================
    Axon Kernel - X86 ACPI PM Timer Driver
    2021, Zachary Berry
    axon/private/axon/arch_x86/acpi_timer_driver.h
==============================================================*/

#pragma once
#include "axon/system/timers.h"


/*
    ACPI PM Timer Driver State
*/
struct axk_x86_acpi_timer_driver_t
{
    struct axk_timer_driver_t func_table;
};


/*
    axk_x86_create_acpi_pmtimer_driver
    * Private Function (x86 only)
    * Creates an ACPI Power-Managment Timer
*/
struct axk_timer_driver_t* axk_x86_create_acpi_timer_driver( void );
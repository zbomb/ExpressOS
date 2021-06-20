/*==============================================================
    Axon Kernel - Architecture Function Library
    2021, Zachary Berry
    axon/private/axon/arch.h
==============================================================*/

#pragma once
#include <stdint.h>

extern uint64_t axk_disable_interrupts( void );
extern void axk_restore_interrupts( uint64_t );
extern uint64_t axk_enable_interrupts( void );

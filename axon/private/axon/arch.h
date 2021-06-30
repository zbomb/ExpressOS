/*==============================================================
    Axon Kernel - Architecture Function Library
    2021, Zachary Berry
    axon/private/axon/arch.h
==============================================================*/

#pragma once
#include <stdint.h>

/*
    Arch Functions
    * Implemented by ASM/C in arch_*\
*/
uint64_t axk_disable_interrupts( void );
void axk_restore_interrupts( uint64_t );
uint64_t axk_enable_interrupts( void );
__attribute__((noreturn)) void axk_halt( void );
uint32_t axk_get_processor_id( void );
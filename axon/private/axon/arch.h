/*==============================================================
    Axon Kernel - Architecture Function Library
    2021, Zachary Berry
    axon/private/axon/arch.h
==============================================================*/

#pragma once
#include <stdint.h>
#include <stdbool.h>

/*
    Arch Functions
    * Implemented by ASM/C in arch_*\
*/
uint64_t axk_disable_interrupts( void );


void axk_restore_interrupts( uint64_t );


uint64_t axk_enable_interrupts( void );


__attribute__((noreturn)) void axk_halt( void );


bool axk_start_aux_processors( uint32_t* out_cpu_count );
/*==============================================================
    Axon Kernel - Architecture Function Library
    2021, Zachary Berry
    axon/private/axon/arch.h
==============================================================*/

#pragma once
#include <stdint.h>
#include <stdbool.h>

/*
    Structures
*/
struct axk_cpu_local_storage_t
{
    void* this_address;
    uint32_t os_identifier;
    uint32_t arch_identifier;
    void* local_scheduler;
    uint32_t domain;
    uint32_t clock_domain;
};

/*
    Arch Functions
    * Implemented by ASM/C in arch_*\
*/
uint64_t axk_disable_interrupts( void );


void axk_restore_interrupts( uint64_t );


uint64_t axk_enable_interrupts( void );


__attribute__((noreturn)) void axk_halt( void );


struct axk_cpu_local_storage_t* axk_get_cpu_local_storage( void );


static inline uint32_t axk_get_cpu_id( void )
{
    return axk_get_cpu_local_storage()->os_identifier;
}
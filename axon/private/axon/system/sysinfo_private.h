/*==============================================================
    Axon Kernel - System Info (Private Header)
    2021, Zachary Berry
    axon/private/axon/system/sysinfo.h
==============================================================*/

#pragma once
#include "axon/kernel/kernel.h"
#include "axon/system/sysinfo.h"

/*
    axk_sysinfo_init 
    * Private Function
    * Initializes the system info container
*/
bool axk_sysinfo_init( void );

/*
    axk_counters_init
    * Private Function
    * Initializes system info counters
    * This is a seperate function, because it can be called before the memory manager is setup
    * The memory manager uses these counters to help track stats for debug
*/
void axk_counters_init( void );

/*
    axk_sysinfo_write
    * Private Function
    * Write a system info frame to the container to the specified index/sub-index combination
*/
void axk_sysinfo_write( uint32_t index, uint32_t sub_index, void* ptr_data, uint64_t data_size );

/*
    axk_counter_increment
    * Private Function
    * Increments a system-wide counter
    * Atomic
    * Returns the new value
*/
uint64_t axk_counter_increment( uint32_t index, uint64_t diff );

/*
    axk_counter_decrement
    * Private Function
    * Decrements a system-wide counter
    * Atomic
    * Returns the new value
*/
uint64_t axk_counter_decrement( uint32_t index, uint64_t diff );

/*
    axk_counter_write
    * Private Function
    * Writes a new value to a system-wide counter
    * Atomic
    * Returns the input value
*/
uint64_t axk_counter_write( uint32_t index, uint64_t value );


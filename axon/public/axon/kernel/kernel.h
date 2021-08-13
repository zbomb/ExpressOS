/*==============================================================
    Axon Kernel - Main Kernel Header
    2021, Zachary Berry
    axon/public/axon/kernel/kernel.h
==============================================================*/

#pragma once

// ---------------------------- Global Headers ----------------------------
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>


// ---------------------------- Address Space Layout ----------------------------
#define AXK_KERNEL_VA_PHYSICAL  0xFFFF800000000000
#define AXK_KERNEL_VA_HEAP      0xFFFFC00000000000
#define AXK_KERNEL_VA_SHARED    0xFFFFE00000000000
#define AXK_KERNEL_VA_IMAGE     0xFFFFFFFF80000000

#define AXK_USER_VA_IMAGE       0x100000000
#define AXK_USER_VA_SHARED      0x400000000000
#define AXK_USER_VA_STACKS      0x7F0000000000


/*
    Macros
*/
#define AXK_ZERO_MEM( _obj_ ) memset( &_obj_, 0, sizeof( _obj_ ) )
#define AXK_EXTRACT( _BF_, _START_, _END_ ) ( ( _BF_ & ( ( ( 1 << ( _END_ - _START_ ) ) - 1 ) << _START_ ) ) >> _START_ )


// ---------------------------- Utility Functions ----------------------------

/*
    axk_interrupts_disable
    * Stops any interrupts from being raised
    * Returns the previous interrupts state, so the previous state can be restored later (if desired)
*/
uint64_t axk_interrupts_disable( void );

/*
    axk_interrupts_restore
    * Restores a previous interrupt state
*/
void axk_interrupts_restore( uint64_t );

/*
    axk_interrupts_enable
    * Enables interrupt generation, returns the new interrupt state
*/
uint64_t axk_interrupts_enable( void );

/*
    axk_halt
    * Halts the execution of current processor
*/
__attribute__((noreturn)) void axk_halt( void );
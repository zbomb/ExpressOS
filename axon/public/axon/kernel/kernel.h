/*==============================================================
    Axon Kernel - Main Kernel Header
    2021, Zachary Berry
    axon/public/axon/kernel/kernel.h
==============================================================*/

#pragma once

/*
    Global Headers
*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/*
    Address Space Layout Constants
*/
#define AXK_KERNEL_VA_PHYSICAL  0xFFFF800000000000UL
#define AXK_KERNEL_VA_HEAP      0xFFFFC00000000000UL
#define AXK_KERNEL_VA_SHARED    0xFFFFE00000000000UL
#define AXK_KERNEL_VA_IMAGE     0xFFFFFFFF80000000UL

#define AXK_USER_VA_IMAGE       0x100000000UL
#define AXK_USER_VA_SHARED      0x400000000000UL
#define AXK_USER_VA_STACKS      0x7F0000000000UL

/*
    Other Constants
*/
#ifdef __x86_64__
#define AXK_PAGE_SIZE       0x1000UL
#define AXK_HUGE_PAGE_SIZE  0x200000UL
#endif

#define AXK_PROCESS_INVALID     0UL
#define AXK_PROCESS_KERNEL      1UL


/*
    Macros
*/
#define AXK_ZERO_MEM( _obj_ )                   memset( &_obj_, 0, sizeof( _obj_ ) )
#define AXK_EXTRACT( _BF_, _START_, _END_ )     ( ( _BF_ & ( ( ( 1 << ( _END_ - _START_ ) ) - 1 ) << _START_ ) ) >> _START_ )
#define AXK_CHECK_FLAG( _bf_, _flag_ )          ( ( _bf_ & ( _flag_ ) ) == ( _flag_ ) )
#define AXK_CHECK_ANY_FLAG( _bf_, _flags_ )     ( ( _bf_ & ( _flags_ ) ) != 0 )
#define AXK_SET_FLAG( _bf_, _flag_ )            ( _bf_ |= ( _flag_ ) )
#define AXK_CLEAR_FLAG( _bf_, _flag_ )          ( _bf_ &= ~( _flag_ ) )


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

/*
    axk_get_kernel_offset
    * Gets the virtual offset of the kernel image
*/
uint64_t axk_get_kernel_offset( void );

/*
    axk_get_kernel_size
    * Gets the size of the kernel image in bytes
*/
uint64_t axk_get_kernel_size( void );
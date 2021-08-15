/*==============================================================
    Axon Kernel - Kernel Panic System
    2021, Zachary Berry
    axon/public/axon/kernel/panic.h
==============================================================*/

#pragma once
#include "axon/kernel/kernel.h"

/*
    axk_panic_init
    * Initializes the panic system state
    * Must be called before any panics are able to be called
*/
void axk_panic_init( void );

/* 
    axk_panic
    * Kernel panic function, called whenever an error occurs that is non-recoverable
    * Will use the 'basic terminal' system to write out an error display to the string, inclduing the message supplied as a parameter
    * This function will never returns, and will stop the execution of all processors in the system as soon as possible
*/
__attribute__((noreturn, noinline)) void axk_panic( const char* str );

/*
    axk_panic_begin
    * More advanced kernel panic interface, a more detailed message that includes numbers, text and/or processor state
    * After calling this function, use the following functions to write the error message:
       - axk_panic_prints       (Write a string to the error message section)
       - axk_panic_printn       (Write an unsigned integer to the error message section)
       - axk_panic_printh       (Write a hex number to the error message section)
       - axk_panic_set_pstate   (Indicate processor information structure to display on the panic screen)
    * Then, this sequence MUST be terminated by a call to 'axk_panic_end'!!!!
    * This function call will stop the other processors and start displaying the static portions of the panic screen
*/
__attribute__((noinline)) void axk_panic_begin( void );

/*
    axk_panic_prints
    * After calling 'axk_panic_begin', this function will write a string to the error message section of the panic screen, at the current position
    * Once all 'print' functions are done, the caller MUST end the sequence with a call to 'axk_panic_end'
*/
void axk_panic_prints( const char* str );

/*
    axk_panic_printn
    * After calling 'axk_panic_begin', this function will write an unsigned number to the error message section of the panic screen, at the current position
    * Once all 'print' functions are done, the caller MUST end the sequence with a call to 'axk_panic_end'
*/
void axk_panic_printn( uint64_t num );

/*
    axk_panic_printh
    * After calling 'axk_panic_begin', this function will write a hex number to the error message section of the panic screen, at the current position
    * Once all 'print' functions are done, the caller MUST end the sequence with a call to 'axk_panic_end'
*/
void axk_panic_printh( uint64_t num, bool b_leading_zeros );

/*
    axk_panic_set_pstate
    * After calling 'axk_panic_begin', this function can be called to display processor state on the panic screen
    * Once all 'print' (and set_pstate) function calls are complete, the CALLER must call 'axk_panic_end'
*/
void axk_panic_set_pstate( void* pstate );

/*
    axk_panic_end
    * Complete panic sequence, halts the current processor
*/
__attribute__((noreturn)) void axk_panic_end( void );
/*==============================================================
    Axon Kernel - Kernel Panic System
    2021, Zachary Berry
    axon/public/axon/kernel/panic.h
==============================================================*/

#pragma once

/* 
    axk_panic
    * Kernel panic function, called whenever an error occurs that is non-recoverable
    * Will use the 'basic terminal' system to write out an error display to the string, inclduing the message supplied as a parameter
    * This function will never returns, and will stop the execution of all processors in the system as soon as possible
*/
__attribute__((noreturn, noinline)) void axk_panic( const char* str );

/*
    axk_panic_init
    * Initializes the panic system state
    * Must be called before any panics are able to be called
*/
void axk_panic_init( void );
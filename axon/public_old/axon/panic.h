/*==============================================================
    Axon Kernel - Panic
    2021, Zachary Berry
    axon/public/axon/panic.h
==============================================================*/

#pragma once
#include "axon/boot/basic_out.h"

/*
    axk_panic
    * The kernal panic function, called when we hit a critical, nonrecoverable error
    * Prints error info to the screen, along with a message passed by the caller
    * This function will never return
    
    Parameters:
        str -> Message to be printed to the screen
*/
__attribute__((noreturn, noinline)) void axk_panic( const char* str );
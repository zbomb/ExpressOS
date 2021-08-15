/*==============================================================
    Axon Kernel - Panic System (Private Header)
    2021, Zachary Berry
    axon/private/axon/kernel/panic_private.h
==============================================================*/

#pragma once
#include "axon/kernel/panic.h"


/*
    axk_panic_init
    * Private Function
    * Initializes the spin-lock used for the panic function
*/
void axk_panic_init( void );
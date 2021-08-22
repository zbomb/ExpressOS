/*==============================================================
    Axon Kernel - Basic Terminal Library
    2021, Zachary Berry
    axon/public/axon/gfx/basic_terminal.h
==============================================================*/

#pragma once
#include "axon/gfx/basic_terminal.h"


/*
    axk_basicterminal_init
    * Private Function
    * Initializes the 'basic terminal' system, so simple text can be written to the console before full graphics
      drivers are able to be properly loaded. Also used by the panic system.
    * NOTE: By default, the mode is set to 'BASIC_TERMINAL_MODE_CONSOLE'
*/
bool axk_basicterminal_init( struct tzero_payload_parameters_t* in_params );

/*
    axk_basicterminal_update_pointers
    * Private Function
    * Called by the virtual memory system after initializing
    * This is because, the virtual memory system is going to remove the identity mappings used by UEFI
    * This isnt the cleanest solution.. but for now its functional
*/
void axk_basicterminal_update_pointers( void );
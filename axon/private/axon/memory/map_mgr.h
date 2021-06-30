/*==============================================================
    Axon Kernel - Memory Map Manager
    2021, Zachary Berry
    axon/public/axon/memory/map_mgr.h
    
    * Definitions located in 'axon/source/memory/kmap.c'
==============================================================*/

#pragma once
#include "axon/config.h"


/*
    axk_mapmgr_init
    * Initializes the virtual memory system
    * Returns if the call was successful
*/
bool axk_mapmgr_init( void );

/*
    axk_mapmgr_get_table
    * Returns the address of the Page Map Level 4 (in x86), for the kernels mappings
*/
void* axk_mapmgr_get_table( void );
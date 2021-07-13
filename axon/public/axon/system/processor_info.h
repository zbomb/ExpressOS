/*==============================================================
    Axon Kernel - Processor Info Functions
    2021, Zachary Berry
    axon/public/system/processor_info.h
==============================================================*/

#pragma once
#include "axon/config.h"


/*
    axk_processor_get_id
    * Gets the ID of the core were currently executing on
*/
uint32_t axk_processor_get_id( void );

/*
    axk_processor_get_count
    * Gets the total number of processor cores on the system
*/
uint32_t axk_processor_get_count( void );

/*
    axk_processor_get_vendor
    * Gets the processor vendor
*/
const char* axk_processor_get_vendor( void );

/*
    axk_processor_get_boot_id
    * Gets the ID of the core that booted the system
*/
uint32_t axk_processor_get_boot_id( void );

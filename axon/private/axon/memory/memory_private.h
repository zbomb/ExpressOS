/*==============================================================
    Axon Kernel - Memory System (Private Header)
    2021, Zachary Berry
    axon/public/axon/memory/memory_private.h
==============================================================*/

#pragma once
#include "axon/kernel/kernel.h"
#include "axon/kernel/boot_params.h"


/*
    axk_page_allocator_init
    * Private Function
    * Initializes the page allocator system
*/
void axk_page_allocator_init( struct tzero_payload_parameters_t* in_params );

/*
    axk_page_reclaim
    * Private Function
    * Reclaims a type of page (ACPI or Bootloader)
*/
uint64_t axk_page_reclaim( uint8_t target_state );


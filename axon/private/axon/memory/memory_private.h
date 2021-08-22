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
    axk_page_allocator_update_pointers
    * Private Function
    * Updates the pointers used by the page allocator system
    * Before the memory mappings system is initialized, we were using the identity mappings leftover from UEFI
    * Those mappings get deleted, so we need to update the pointers to the AXK_KERNEL_VA_PHYSICAL range so nothing breaks
*/
void axk_page_allocator_update_pointers( void );

/*
    axk_page_reclaim
    * Private Function
    * Reclaims a type of page (ACPI or Bootloader)
*/
uint64_t axk_page_reclaim( uint8_t target_state );

/*
    axk_page_render_debug
    * DEBUG PRIVATE FUNCTION
    * Will be removed soon
    * Draws a visual representation of the physical memory to the screen
    * Halts the system
*/
//void axk_page_render_debug( void );

/*
    axk_kmap_init
    * Private Function
    * Initializes the kernel memory map
*/
void axk_kmap_init( struct tzero_payload_parameters_t* in_params );


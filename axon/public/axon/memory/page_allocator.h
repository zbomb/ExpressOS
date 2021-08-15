/*==============================================================
    Axon Kernel - Page Allocator
    2021, Zachary Berry
    axon/public/axon/memory/page_allocator.h
==============================================================*/

#pragma once
#include "axon/kernel/kernel.h"

/*
    Constants
*/
#define AXK_PAGE_STATE_RESERVED     0x00
#define AXK_PAGE_STATE_AVAILABLE    0x01
#define AXK_PAGE_STATE_LOCKED       0x02
#define AXK_PAGE_STATE_ACPI         0x03
#define AXK_PAGE_STATE_BOOTLOADER   0x04

#define AXK_PAGE_TYPE_OTHER         0x00
#define AXK_PAGE_TYPE_PAGE_TABLE    0x01
#define AXK_PAGE_TYPE_HEAP          0x02
#define AXK_PAGE_TYPE_IMAGE         0x04
#define AXK_PAGE_TYPE_SHARED        0x08

#define AXK_PAGE_FLAG_NONE          0x00
#define AXK_PAGE_FLAG_CLEAR         0x01
#define AXK_PAGE_FLAG_PREFER_HIGH   0x02
#define AXK_PAGE_FLAG_CONSECUTIVE   0x04
#define AXK_PAGE_FLAG_KERNEL_REL    0x08
/*
    axk_page_acquire
    * Finds an available page, or a group of pages, locks their status and returns the list of page identifiers
*/
bool axk_page_acquire( uint64_t count, uint64_t* out_page_list, uint32_t process_id, uint8_t type, uint32_t flags );

/*
    axk_page_lock
    * Locks a list of specific pages
    * If any of the pages in the list specified are unable to be locked, the call will fail
*/
bool axk_page_lock( uint64_t count, uint64_t* in_page_list, uint32_t process, uint8_t type, uint32_t flags );

/*
    axk_page_release
    * Releases a list of pages, and resets their status back to available
    * If any of the specified pages were unable to be released, the call will fail
    * Kernel pages can only be released if the 'AXK_PAGE_FLAG_KERNEL_REL' flag is specified!
*/
bool axk_page_release( uint64_t count, uint64_t* in_page_list, uint32_t flags );

/*
    axk_page_release_s
    * Releases a list of pages if and only if, the process identifier specified matches the pages associated process identifier
*/
bool axk_page_release_s( uint64_t count, uint64_t* in_page_list, uint32_t process, uint32_t flags );

/*
    axk_page_status
    * Gets the current status of a single page
*/
bool axk_page_status( uint64_t in_page, uint32_t* out_process_id, uint8_t* out_state, uint8_t* out_type );

/*
    axk_page_find
    * Finds all pages allocated to the specified process
    * If 'out_page_list' is NULL, the number of pages owned by the process will still be returned
*/
bool axk_page_find( uint32_t target_process_id, uint64_t* out_count, uint64_t* out_page_list );

/*
    axk_page_count
    * Gets the number of potentially available pages in the system
    * Might not actually reflect the exact amount of installed memory, if there are permanently reserved pages at the end
      of the memory space, those pages wont be managed by this system, and therefore arent included in this count
*/
uint64_t axk_page_count( void );
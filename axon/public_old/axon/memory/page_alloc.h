/*==============================================================
    Axon Kernel - Physical Page Allocation
    2021, Zachary Berry
    axon/public/axon/memory/page_alloc.h

    * Definitions located in 'axon/source/memory/page_mgr.c'
==============================================================*/

#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "axon/config.h"


/*
    Flags
*/

/*
    axk_page_acquire
    * Finds a free page, or multiple free pages, locks them and returns the list of page identifiers

    Parameters:
        uint64_t count          -> The number of pages you wish to acquire
        AXK_PAGE_ID* out_list   -> The list of pages returned from the call. The caller must provide a uint64_t array of size 'count', will fail otherwise
        uint32_t process        -> The process ID of whatever process actually 'owns' this page
        AXK_PAGE_FLAGS flags    -> Flags for either the page itself, or to modify the behavior of this function

    Returns: bool
        true, if the number of desired pages were acquired
        false, if the total number of pages couldnt be acquired, or if 'out_list' was null
*/
bool axk_page_acquire( uint64_t count, AXK_PAGE_ID* out_list, uint32_t process, AXK_PAGE_FLAGS flags );

/*
    axk_page_lock
    * Locks a specific, or a range of specific pages 

    Parameters:
        uint64_t count          -> The number of pages in 'in_list' that you want to lock
        AXK_PAGE_ID* in_list    -> An array of page identifiers of at least 'count' elements
        uint32_t process        -> The process ID of whatever process actually 'owns' this page
        AXK_PAGE_FLAGS flags    -> Flags for either the page itself, or to modify the behavior of this function

    Returns: bool
        true, if all pages were able to be locked
        false, if any of the pages were unable to be locked, or if 'in_list' was null
*/
bool axk_page_lock( uint64_t count, AXK_PAGE_ID* in_list, uint32_t process, AXK_PAGE_FLAGS flags );

/*
    axk_page_release
    * Release ownership of a page, or a list of pages

    Parameters:
        uint64_t count          -> The number of pages in 'in_list' that you wish to release
        AXK_PAGE_ID* in_list    -> The list of pages to release, must be at least 'count' elements
        bool b_allowkernel      -> If kernel pages are able to be released (kernel heap, kernel shared)

    Returns: bool
        true, if the pages were all able to be released
        false, if any of the pages were unable to be released, or if 'in_list' was null
*/
bool axk_page_release( uint64_t count, AXK_PAGE_ID* in_list, bool b_allowkernel );

/*
    axk_page_status
    * Gets the current status, and owning process of a single page

    Parameters:
        AXK_PAGE_ID page_id         -> The page identifier were targetting
        uint32_t* out_process       -> If successful, will have the process that owns the page written to it
        AXK_PAGE_FLAGS* out_flags   -> If successful, will have the flags associated with the page written to it

    Returns: bool
        true, if the page is a valid page
        false, if the page is not a valid page
*/
bool axk_page_status( AXK_PAGE_ID page_id, uint32_t* out_process, AXK_PAGE_FLAGS* out_flags );

/*
    axk_page_freeproc
    * Releases all pages associated with a process ID

    Parameters:
        uint32_t process    -> The process ID associated with the pages we wish to free all of

    Returns: uint64_t
        The number of pages that were released
*/
uint64_t axk_page_freeproc( uint32_t process );
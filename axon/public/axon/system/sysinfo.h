/*==============================================================
    Axon Kernel - System Info 
    2021, Zachary Berry
    axon/public/axon/system/sysinfo.h
==============================================================*/

#pragma once
#include "axon/kernel/kernel.h"

/*
    Constants
*/

// SysInfo Pages
#define AXK_SYSINFO_GENERAL     0x00
#define AXK_SYSINFO_PROCESSOR   0x01
#define AXK_SYSINFO_TOPOLOGY    0x02

// SysInfo Counters
#define AXK_COUNTER_AVAILABLE_PAGES     0x00
#define AXK_COUNTER_RESERVED_PAGES      0x01
#define AXK_COUNTER_KERNEL_PAGES        0x02
#define AXK_COUNTER_USER_PAGES          0x03
#define AXK_COUNTER_EXT_CLOCK_TICKS     0x04
#define AXK_COUNTER_MAX_INDEX           0x04

// Other Constants...
#define AXK_PROCESSOR_TYPE_NORMAL       0x00
#define AXK_PROCESSOR_TYPE_LOW_POWER    0x01

/*
    Structures
*/
struct axk_sysinfo_general_t
{
    uint64_t total_memory;
    uint32_t cpu_count;
    uint32_t bsp_id;
    uint32_t cache_count;

    // TODO: Additional fields?
};

struct axk_sysinfo_processor_t
{
    uint32_t identifier;
    uint8_t type;
    uint32_t domain;
    uint32_t clock_domain;
    uint32_t package_id;
    uint32_t core_id;
    uint32_t smt_id;
    uint32_t cache_l1_id;
    uint32_t cache_l2_id;
    uint32_t cache_l3_id;
    uint32_t cache_l4_id;
    uint32_t cache_l1_size;
    uint32_t cache_l2_size;
    uint32_t cache_l3_size;
    uint32_t cache_l4_size;
};

struct axk_sysinfo_topology_domain_t
{

};

struct axk_sysinfo_topology_t
{
    uint32_t domain_count;
    struct axk_sysinfo_topology_domain_t* domain_list;
};

/*
    axk_sysinfo_query
    * Search the system info container for the specified index
    * Optionally: You can specify a subindex, used for some types of info like processor info,
      where there are multiple instances of the info type
    * You must provide a pointer to the structure to copy the information into
    * Also, you must provide the size of this structure, to help validate the request and avoid page faults
    * If 'out_data' is NULL, this function will simply check if the 'index/sub-index' combination is valid (and data_size matches)
    * Otherwise, 'data_size' must match the size of data stored in the container
*/
bool axk_sysinfo_query( uint32_t index, uint32_t sub_index, void* out_data, uint64_t data_size );

/*
    axk_counter_read
    * Reads a system-wide atomic counter
    * If the counter index is invalid, returns 0UL
*/
uint64_t axk_counter_read( uint32_t index );
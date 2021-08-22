/*==============================================================
    Axon Kernel - Memory Mapping
    2021, Zachary Berry
    axon/public/axon/memory/memory_map.h
==============================================================*/

#pragma once
#include "axon/kernel/kernel.h"
#include "axon/library/spinlock.h"


/*
    Flags
*/
#define AXK_MAP_FLAG_NONE           0x00
#define AXK_MAP_FLAG_READ_ONLY      0x01
#define AXK_MAP_FLAG_NO_EXEC        0x02
#define AXK_MAP_FLAG_GLOBAL         0x04
#define AXK_MAP_FLAG_NO_CACHE       0x08
#define AXK_MAP_FLAG_KERNEL_ONLY    0x10

/*
    axk_memory_map_t (Structure)
    * Holds a virtual memory map that can be applied to the kernel, or to a user process
*/
struct axk_memory_map_t
{
    struct axk_spinlock_t lock;
    uint32_t process_id;

    #ifdef __x86_64__
    uint64_t* pml4;
    #else
    void* map;
    #endif
};

/*
    axk_memory_map_create
    * Create a new memory map instance
*/
bool axk_memory_map_create( struct axk_memory_map_t* in_map, uint32_t process_id );

/*
    axk_memory_map_destroy
    * Destroy an existing memory map
*/
void axk_memory_map_destroy( struct axk_memory_map_t* in_map );

/*
    axk_memory_map_lock
    * Locks the memory map, so no other threads/processors can access the map while operations
      are occuring, the map doesnt automatically lock during operations and this needs to be manually
      called before performing any operations
    * Once all operations are complete, be sure to call 'axk_memory_map_unlock'
    * NOTE: The memory maps should be locked for as LITTLE time as possible!
*/
void axk_memory_map_lock( struct axk_memory_map_t* in_map );

/*
    axk_memory_map_unlock
    * Unlocks the memory map, so other threads/processors can now access it
    * NOTE: The memory maps should be locked for as LITTLE time as possible!
*/
void axk_memory_map_unlock( struct axk_memory_map_t* in_map );

/*
    axk_memory_map_add
    * Adds a memory map entry, optionally allowing overwriting
    * If 'out_page_id' is NULL, then overwriting is not allowed, and if theres an existing entry, this call will fail
    * If 'out_page_id' is NOT NULL, then overwriting IS allowed, and the overwritten page ID will be written to 'out_page_id'
    * 'in_vaddr' must be page aligned
*/
bool axk_memory_map_add( struct axk_memory_map_t* in_map, uint64_t in_vaddr, uint64_t in_page_id, uint64_t* out_page_id, uint32_t flags );

/*
    axk_memory_map_remove
    * Removes a memory map entry
    * 'out_page_id' must be NON NULL, the removed page identifier will be written to it
    * 'in_vaddr' must be page aligned
*/
bool axk_memory_map_remove( struct axk_memory_map_t* in_map, uint64_t in_vaddr, uint64_t* out_page_id );

/*
    axk_memory_map_translate
    * Translate a virtual memory address using a memory map
    * Also returns the flags associated with the page
*/
bool axk_memory_map_translate( struct axk_memory_map_t* in_map, uint64_t in_addr, uint64_t* out_addr, uint32_t* out_flags );

/*
    axk_memory_map_search
    * Searching for where a physical page is mapped in the memory map
    * Also returns the flags associated with the mapping
*/
bool axk_memory_map_search( struct axk_memory_map_t* in_map, uint64_t in_page_id, uint64_t* out_virt_addr, uint32_t* out_flags );

/*
    axk_memory_map_copy
    * Copies a mapping from one memory map to another memory map
    * Also copies over the flags
    * Will fail if there is an existing page in the destination map at the target address
*/
bool axk_memory_map_copy( struct axk_memory_map_t* src_map, struct axk_memory_map_t* dst_map, uint64_t src_vaddr, uint64_t dst_vaddr );

/*
    axk_memory_map_copy_range
    * Copies a range of mappings from one memory map to another memory map
    * Also copies over the flags
    * Copies from 'start_src_vaddr' (inclusive) to 'end_src_vaddr' (exclusive)
    * Will fail if there are any existing pages in the destination map within the target address range
    * Any existing pages should be first removed
*/
bool axk_memory_map_copy_range( struct axk_memory_map_t* src_map, struct axk_memory_map_t* dst_map, uint64_t start_src_vaddr, uint64_t end_src_vaddr, uint64_t dest_vaddr );

/*
    axk_kmap_get
    * Gets the kernel memory map instance
*/
struct axk_memory_map_t* axk_kmap_get( void );
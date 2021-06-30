/*==============================================================
    Axon Kernel - Boot Parameters
    2021, Zachary Berry
    axon/private/axon/boot/boot_params.h
==============================================================*/

#pragma once
#include <stdint.h>
#include <stdbool.h>

/*
    Constants
*/
#define AXK_BOOT_MEMORYMAP_MAX_ENTRIES 128

#ifndef AXK_PACK
#define AXK_PACK __attribute__((__packed__))
#endif

enum AXK_MEMORYMAP_ENTRY_STATUS
{
    MEMORYMAP_ENTRY_STATUS_AVAILABLE    = 0,
    MEMORYMAP_ENTRY_STATUS_RESERVED     = 1,
    MEMORYMAP_ENTRY_STATUS_KERNEL       = 2,
    MEMORYMAP_ENTRY_STATUS_ACPI         = 3,
    MEMORYMAP_ENTRY_STATUS_RAMDISK      = 4
};

/*
    Structures
*/
struct axk_bootparams_memorymap_entry_t
{
    uint64_t begin;
    uint64_t end;
    enum AXK_MEMORYMAP_ENTRY_STATUS status;
    uint32_t flags;

} AXK_PACK;

struct axk_bootparams_memorymap_t
{
    uint32_t entry_count;
    uint64_t kernel_offset;
    uint64_t kernel_size;
    uint64_t initrd_offset;
    uint64_t initrd_size;
    uint64_t total_mem;

    struct axk_bootparams_memorymap_entry_t entry_list[ AXK_BOOT_MEMORYMAP_MAX_ENTRIES ];
};

enum AXK_FRAMEBUFFER_FORMAT
{
    FRAMEBUFFER_FORMAT_RGB8 = 0,
    FRAMEBUFFER_FORMAT_RBG8 = 1,
    FRAMEBUFFER_FORMAT_GRB8 = 2,
    FRAMEBUFFER_FORMAT_GBR8 = 3,
    FRAMEBUFFER_FORMAT_BRG8 = 4,
    FRAMEBUFFER_FORMAT_BGR8 = 5
};

struct axk_bootparams_framebuffer_t
{
    uint32_t width;
    uint32_t height;
    uint32_t scanline_size;
    uint8_t bits_per_pixel;
    enum AXK_FRAMEBUFFER_FORMAT format;
    void* buffer;
};


/*
    axk_bootparams_get_memorymap
    * Internal Function
    * Returns a pointer to a structure describing the system memory map
*/
const struct axk_bootparams_memorymap_t* axk_bootparams_get_memorymap( void );

/*
    axk_bootparams_get_framebuffer
    * Internal Function
    * Returns a pointer to a structure describing the framebuffer
*/
const struct axk_bootparams_framebuffer_t* axk_bootparams_get_framebuffer( void );



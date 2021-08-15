/*==============================================================
    Axon Kernel - Boot Parameters Structures
    2021, Zachary Berry
    axon/private/axon/kernel/boot_params.h
==============================================================*/

#pragma once
#include "axon/kernel/kernel.h"

/*
    Constants
*/
#define TZERO_MAGIC_VALUE       0x4C4946544F464621UL
#define TZERO_ARCH_CODE_X86     0x80000000U

/*
    T-0 Framebuffer Structure
*/
enum tzero_pixel_format_t
{
    PIXEL_FORMAT_INVALID = 0,
    PIXEL_FORMAT_RGBX_32 = 1,
    PIXEL_FORMAT_BGRX_32 = 2,
    PIXEL_FORMAT_BITMASK = 3
};

struct tzero_resolution_t
{
    uint32_t width;
    uint32_t height;
    uint32_t pixels_per_scanline;
    uint32_t index;
    uint8_t red_bit_width;
    uint8_t green_bit_width;
    uint8_t blue_bit_width;
    uint8_t red_shift;
    uint8_t green_shift;
    uint8_t blue_shift;
    uint8_t mode;
    uint8_t _pad_;
};

struct tzero_framebuffer_t
{
    uint64_t phys_addr;
    size_t size;
    struct tzero_resolution_t resolution;
};

/*
    T-0 Memory Map
*/
enum tzero_memory_status_t
{
    TZERO_MEMORY_RESERVED       = 0,
    TZERO_MEMORY_AVAILABLE      = 1,
    TZERO_MEMORY_ACPI           = 2,
    TZERO_MEMORY_BOOTLOADER     = 3,
    TZERO_MEMORY_MAPPED_IO      = 4
};

struct tzero_memory_entry_t
{
    uint64_t base_address;
    uint64_t page_count;
    uint32_t type;
    uint32_t _pad_;
};

struct tzero_memory_map_t
{
    struct tzero_memory_entry_t* list;
    uint32_t count;
}; 

/*
    Generic Payload Parameters
*/
struct tzero_payload_parameters_t
{
    uint64_t magic_value;
    void( *fn_on_success )( void );
    void( *fn_on_error )( const char* );
    struct tzero_framebuffer_t framebuffer;
    struct tzero_memory_map_t memory_map;
    struct tzero_resolution_t* available_resolutions;
    uint32_t resolution_count;
};
 

/*=========================================================================================
    X86-64 Specific Structures
==========================================================================================*/
#ifdef __x86_64__

/*
    T-0 ACPI Info
*/
struct tzero_acpi_info_t
{
    uint64_t rsdp_phys_addr;
    bool b_rsdp_new_version;
};

/*
    x86 Specific Payload Parameters
*/
struct tzero_x86_payload_parameters_t
{
    uint64_t magic_value;
    uint32_t arch_code;
    struct tzero_acpi_info_t acpi;
};

#endif


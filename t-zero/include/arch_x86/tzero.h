/*==============================================================
    T-0 Bootloader - General Header
    2021, Zachary Berry
    t-zero/include/tzero.h
==============================================================*/

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <efi.h>
#include <efilib.h>
#include <elf.h>

/*
    Console Colors
*/
#define TZERO_COLOR_BLACK           0x00
#define TZERO_COLOR_BLUE            0x01
#define TZERO_COLOR_GREEN           0x02
#define TZERO_COLOR_CYAN            0x03
#define TZERO_COLOR_RED             0x04
#define TZERO_COLOR_MAGENTA         0x05
#define TZERO_COLOR_BROWN           0x06
#define TZERO_COLOR_LIGHTGRAY       0x07
#define TZERO_COLOR_DARKGRAY        0x08
#define TZERO_COLOR_LIGHTBLUE       0x09
#define TZERO_COLOR_LIGHTGREEN      0x0A
#define TZERO_COLOR_LIGHTCYAN       0x0B
#define TZERO_COLOR_LIGHTRED        0x0C
#define TZERO_COLOR_LIGHTMAGENTA    0x0D
#define TZERO_COLOR_YELLOW          0x0E
#define TZERO_COLOR_WHITE           0x0F

#define TZERO_FGBG_COLOR( _FG_, _BG_ ) ( ( _FG_ ) | ( ( _BG_ ) << 4 ) )

/*
    GUIDS
*/
#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    { 0x5B1B31A1, 0x9562, 0x11d2, \
    { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } }

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    { 0x0964e5b22, 0x6459, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

/*
    Other Constants
*/
#define TZERO_MAGIC_VALUE           0x4C4946544F464621UL
#define TZERO_ARCH_CODE_X86         0x80000000U


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
    T-0 ACPI Info
*/
struct tzero_acpi_info_t
{
    uint64_t rsdp_phys_addr;
    bool b_rsdp_new_version;
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

/*
    x86 Specific Payload Parameters
*/
struct tzero_x86_payload_parameters_t
{
    uint64_t magic_value;
    uint32_t arch_code;
    struct tzero_acpi_info_t acpi;
};

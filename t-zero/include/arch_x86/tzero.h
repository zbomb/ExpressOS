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
#define TZERO_MAGIC_VALUE   0x4C4946544F464621UL
#define TZERO_KVA_OFFSET    0xFFFFFFFF80000000

/*
    Global Helper Functions
*/
__attribute__((__noreturn__)) void tzero_halt( void );

/*
    Framebuffer Structure
*/
enum efi_pixel_format_t
{
    efi_pixel_rgbx_32   = 0,
    efi_pixel_bgrx_32   = 1
};

struct efi_resolution_t
{
    uint32_t width;
    uint32_t height;
    uint32_t pixels_per_scanline;
    uint32_t index;
    enum efi_pixel_format_t format;
    uint32_t _pad;

};  // Size = 24 bytes

struct efi_framebuffer_t
{
    void* address;
    size_t size;
    struct efi_resolution_t res;
};


/*
    Kernel Parameters
*/
struct tzero_payload_params_t
{
    uint64_t magic_value;
    void* framebuffer_ptr;
    size_t framebuffer_size;
    struct efi_resolution_t framebuffer_res;
    EFI_MEMORY_DESCRIPTOR* memmap_ptr;
    uint32_t memmap_size;
    uint32_t memmap_desc_size;
    void* rsdp_ptr;
    bool b_new_rsdp;
    struct efi_resolution_t* res_list;
    uint32_t res_count;
};

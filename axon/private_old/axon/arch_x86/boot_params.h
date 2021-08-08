/*==============================================================
    Axon Kernel - Boot Parameters (x86)
    2021, Zachary Berry
    axon/private/axon/arch_x86/boot_params.h
==============================================================*/

#pragma once
#ifdef _X86_64_

#include "axon/config.h"

struct axk_bootparams_acpi_t
{
    uint32_t size;
    bool new_version;
    uint8_t data[ 48 ];
};

struct tzero_memory_map_entry_t
{
    uint32_t type;
    uint64_t physical_address;
    uint64_t virtual_address;
    uint64_t page_count;
    uint64_t attributes;
};

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
};

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory
} EFI_MEMORY_TYPE;

struct tzero_payload_params_t
{
    uint64_t magic_value;
    void( *fn_load_success )( void );
    void( *fn_load_failed )( const char*, const char* );
    void* framebuffer_ptr;
    size_t framebuffer_size;
    struct efi_resolution_t framebuffer_res;
    struct tzero_memory_map_entry_t* memmap_ptr;
    uint32_t memmap_size;
    uint32_t memmap_desc_size;
    void* rsdp_ptr;
    bool b_new_rsdp;
    struct efi_resolution_t* res_list;
    uint32_t res_count;
};

/*
    axk_x86_bootparams_parse
    * Internal Function (x86-64 only)
    * Parses information passed to the kernel from the T-0 bootloader
*/
bool axk_x86_bootparams_parse( struct tzero_payload_params_t* ptr_params, struct axk_error_t* out_err );

/*
    axk_bootparams_get_acpi
    * Internal Function
    * Gets a pointer to the ACPI info found when parsing the boot parameters
*/
const struct axk_bootparams_acpi_t* axk_bootparams_get_acpi( void );

#endif
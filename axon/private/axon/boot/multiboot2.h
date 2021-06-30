/*==============================================================
    Axon Kernel - Multiboot2 Parsing
    2021, Zachary Berry
    axon/private/axon/boot/multiboot2.h
==============================================================*/

#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifndef AXK_PACK
#define AXK_PACK __attribute__((__packed__))
#endif

/*
    Constants
*/
enum AXK_MULTIBOOT_MEM_TYPE
{
    MULTIBOOT_MEM_TYPE_AVAIL_RAM        = 0x01,
    MULTIBOOT_MEM_TYPE_AVAIL_ACPI       = 0x03,
    MULTIBOOT_MEM_TYPE_RSVD_HIBERNATE   = 0x04,
    MULTIBOOT_MEM_TYPE_BAD              = 0x05
};

enum AXK_MULTIBOOT_FRAMBUFFER_TYPE
{
    MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED  = 0x00,
    MULTIBOOT_FRAMEBUFFER_TYPE_RGB      = 0x01,
    MULTIBOOT_FRAMEBUFFER_TYPE_TEXT     = 0x02
};

enum AXK_MULTIBOOT_TAG
{
    MULTIBOOT_TAG_END                               = 0x00,
    MULTIBOOT_TAG_BASIC_MEMORY_INFO                 = 0x04,
    MULTIBOOT_TAG_BIOS_BOOT_DEVICE                  = 0x05,
    MULTIBOOT_TAG_BOOT_CMD_LINE                     = 0x01,
    MULTIBOOT_TAG_MODULES                           = 0x03,
    MULTIBOOT_TAG_ELF_SYMBOLS                       = 0x09,
    MULTIBOOT_TAG_MEMORY_MAP                        = 0x06,
    MULTIBOOT_TAG_BOOTLOADER_NAME                   = 0x02,
    MULTIBOOT_TAG_APM_TABLE                         = 0x0A,
    MULTIBOOT_TAG_VBE_INFO                          = 0x07,
    MULTIBOOT_TAG_FRAMEBUFFER_INFO                  = 0x08,
    MULTIBOOT_TAG_EFI32_SYSTABLE_POINTER            = 0x0B,
    MULTIBOOT_TAG_EFI64_SYSTABLE_POINTER            = 0x0C,
    MULTIBOOT_TAG_SMBIOS_TABLES                     = 0x0D,
    MULTIBOOT_TAG_ACPI_OLD_RSDP                     = 0x0E,
    MULTIBOOT_TAG_ACPI_NEW_RSDP                     = 0x0F,
    MULTIBOOT_TAG_NETWORK_INFO                      = 0x10,
    MULTIBOOT_TAG_EFI_MEMORY_MAP                    = 0x11,
    MULTIBOOT_TAG_EFI_BOOT_SERVICES_NOT_TERMINATED  = 0x12,
    MULTIBOOT_TAG_EFI32_IMAGE_HANDLE_POINTER        = 0x13,
    MULTIBOOT_TAG_EFI64_IMAGE_HANDLE_POINTER        = 0x14,
    MULTIBOOT_TAG_IMAGE_LOAD_BASE_PHYS_ADDRESS      = 0x15
};

/*
    Structures
*/
struct axk_multiboot_header_t
{
    uint32_t total_size;
    uint32_t _rsvd_1_;

} AXK_PACK;

struct axk_multiboot_tag_t
{
    uint32_t type;
    uint32_t size;

} AXK_PACK;

struct axk_multiboot_basic_memory_info_t
{
    uint32_t mem_lower;
    uint32_t mem_upper;

} AXK_PACK;

struct axk_multiboot_bios_boot_device_t
{
    uint32_t bios_dev;
    uint32_t partition;
    uint32_t sub_partition;

} AXK_PACK;

struct axk_multiboot_boot_cmdline_t
{
    // Variable length string here...
};

struct axk_multiboot_modules_t
{
    uint32_t mod_start;
    uint32_t mod_end;

    // Variable length string here...

} AXK_PACK;

struct axk_multiboot_elf_symbols_t
{
    uint16_t num;
    uint16_t ent_size;
    uint16_t shndx;
    uint16_t _rsvd_1_;

    // Section headers here.. variable length

} AXK_PACK;

struct axk_multiboot_memorymap_entry_t
{
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t _rsvd_1_;

} AXK_PACK;

struct axk_multiboot_memorymap_t
{
    uint32_t entry_size;
    uint32_t entry_version;
    struct axk_multiboot_memorymap_entry_t* entry_list;

} AXK_PACK;

struct axk_multiboot_bootloader_name_t
{
    // Variable length string here
};

struct axk_multiboot_apm_table_t
{
    uint16_t version;
    uint16_t cseg;
    uint32_t offset;
    uint16_t cseg_16;
    uint16_t dseg;
    uint16_t flags;
    uint16_t cseg_len;
    uint16_t cseg_16_len;
    uint16_t dseg_len;

} AXK_PACK;

struct axk_multiboot_vbe_info_t
{
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint8_t vbe_control_info[ 523 ];
    uint8_t vbe_mode_info[ 256 ];

} AXK_PACK;

struct axk_multiboot_framebuffer_info_t
{
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t reserved;

    // Framebuffer color info here.. variable length

} AXK_PACK;

struct axk_multiboot_framebuffer_colorpalette_t
{
    uint32_t num_colors;
    // Array of axk_multiboot_framebuffer_colorpalette_descriptor_t here

} AXK_PACK;

struct axk_multiboot_framebuffer_colorpalette_descriptor_t
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;

} AXK_PACK;

struct axk_multiboot_framebuffer_rgb_t
{
    uint8_t red_field_pos;
    uint8_t red_mask_size;
    uint8_t green_field_pos;
    uint8_t green_mask_size;
    uint8_t blue_field_pos;
    uint8_t blue_mask_size;

} AXK_PACK;

struct axk_multiboot_efi32_systable_pointer_t
{
    uint32_t pointer;

} AXK_PACK;

struct axk_multiboot_efi64_systable_pointer_t
{
    uint64_t pointer;

} AXK_PACK;

struct axk_multiboot_smbios_tables_t
{
    uint8_t major;
    uint8_t minor;
    uint8_t _rsvd_1_[ 6 ];
    
    // The tables are placed here, requires special handling

} AXK_PACK;

struct axk_multiboot_acpi_old_rsdp_t
{
    // Copy of RSDP (v1) here
};

struct axk_multiboot_acpi_new_rsdp_t
{
    // Copy of RSDP (v2) here
};

struct axk_multiboot_networking_info_t
{
    // DHCP ACK here
};

struct axk_multiboot_efi_memorymap_t
{
    uint32_t descriptor_size;
    uint32_t descriptor_version;

    // EFI memory map info copy here

} AXK_PACK;

struct axk_multiboot_efi32_image_handle_pointer_t
{
    uint32_t pointer;

} AXK_PACK;

struct axk_multiboot_efi64_image_handle_pointer_t
{
    uint64_t pointer;

} AXK_PACK;

struct axk_multiboot_image_base_phys_addr_t
{
    uint32_t base_addr;

} AXK_PACK;



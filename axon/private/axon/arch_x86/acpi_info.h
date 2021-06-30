/*==============================================================
    Axon Kernel - ACPI Info (x86)
    2021, Zachary Berry
    axon/private/axon/arch_x86/acpi_info.h
==============================================================*/

#pragma once
#ifdef _X86_64_
#include "axon/config.h"


/*
    axk_x86_acpi_info_t (STRUCT)
    * Private Structure
    * Contains info about the current system
*/
struct axk_x86_acpi_header_t
{
    char signature[ 4 ];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_identifier[ 6 ];
    char oem_table_identifier[ 8 ];
    uint32_t oem_revision;
    uint32_t creator_identifier;
    uint32_t creator_revision;

} __attribute__(( packed ));

struct axk_x86_rsdp_v1_t
{
    char signature[ 8 ];
    uint8_t checksum;
    char oem_identifier[ 6 ];
    uint8_t revision;
    uint32_t rsdt_address;

} __attribute__(( packed ));
 
struct axk_x86_rsdp_v2_t
{
    char signature[ 8 ];
    uint8_t checksum;
    char oem_identifier[ 6 ];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t ext_checksum;
    char rsvd[ 3 ];

} __attribute__(( packed ));

struct axk_x86_lapic_info_t
{
    uint8_t processor;
    uint8_t id;
    uint32_t flags;
};

struct axk_x86_ioapic_info_t
{
    uint8_t id;
    uint64_t address;
    uint32_t interrupt_base;
    uint8_t interrupt_count;
};

struct axk_x86_int_source_override_t
{
    uint8_t bus;
    uint8_t irq;
    uint32_t global_interrupt;
    uint16_t flags;
};

struct axk_x86_ioapic_nmi_t
{
    uint8_t source;
    uint16_t flags;
    uint32_t global_interrupt;
};

struct axk_x86_lapic_nmi_t
{
    uint8_t processor;
    uint16_t flags;
    uint8_t lint;
};

struct axk_x86_hpet_info_t
{
    uint8_t comparator_count;
    bool is_large_counter;
    bool is_legacy_replacment;
    uint16_t pci_vendor;
    uint8_t addr_space_type;
    uint8_t reg_bit_width;
    uint8_t reg_bit_offset;
    uint64_t address;
    uint8_t hpet_number;
    uint16_t min_tick;
    uint8_t page_protection;
};

struct axk_x86_acpi_info_t
{
    char system_mfgr[ 7 ];
    uint64_t lapic_addr;
    uint32_t max_cpuid;
    uint32_t bsp_id;
    char cpu_vendor[ 12 ];
    uint32_t cpu_count;

    uint32_t lapic_count;
    uint32_t ioapic_count;
    uint32_t source_override_count;
    uint32_t ioapic_nmi_count;
    uint32_t lapic_nmi_count;
    bool b_legacy_pic;

    struct axk_x86_lapic_info_t* lapic_list;
    struct axk_x86_ioapic_info_t* ioapic_list;
    struct axk_x86_int_source_override_t* source_override_list;
    struct axk_x86_ioapic_nmi_t* ioapic_nmi_list;
    struct axk_x86_lapic_nmi_t* lapic_nmi_list;
    struct axk_x86_hpet_info_t* hpet_info;

};

/*
    axk_x86_acpi_parse
    * Private Function
    * Parses the ACPI tables
*/
bool axk_x86_acpi_parse( void );

/*
    axk_x86_acpi_get
    * Private Function
    * Gets the ACPI info structure
*/
struct axk_x86_acpi_info_t* axk_x86_acpi_get( void );


#endif
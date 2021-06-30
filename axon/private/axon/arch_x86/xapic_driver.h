/*==============================================================
    Axon Kernel - X86 XAPIC Driver
    2021, Zachary Berry
    axon/private/axon/arch_x86/xapic_driver.h
==============================================================*/

#pragma once
#include "axon/system/interrupts_mgr.h"
#include "axon/arch_x86/acpi_info.h"
#include "axon/library/spinlock.h"

/*
    Structrures
*/
struct axk_x86_xapic_driver_t
{
    struct axk_interrupt_driver_t func_table;

    uint64_t lapic_address;
    struct axk_spinlock_t ioapic_lock;

    uint32_t ioapic_count;
    uint32_t source_override_count;

    struct axk_x86_ioapic_info_t* ioapic_list;
    struct axk_x86_int_source_override_t* source_override_list;
};


/*
    Functions
*/
struct axk_interrupt_driver_t* axk_x86_create_xapic_driver( void );

void axk_x86_xapic_write_ioapic( struct axk_x86_xapic_driver_t* this, uint8_t id, uint8_t reg, uint32_t val );
uint32_t axk_x86_xapic_read_ioapic( struct axk_x86_xapic_driver_t* this, uint8_t id, uint8_t reg );

void axk_x86_xapic_write_lapic( struct axk_x86_xapic_driver_t* this, uint32_t reg, uint32_t value );
uint32_t axk_x86_xapic_read_lapic( struct axk_x86_xapic_driver_t* this, uint32_t reg );


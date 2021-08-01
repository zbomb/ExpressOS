/*==============================================================
    Axon Kernel - Boot Parameters (x86)
    2021, Zachary Berry
    axon/private/axon/arch_x86/boot_params.h
==============================================================*/

#pragma once
#ifdef _X86_64_

#include <stdint.h>
#include <stdbool.h>

struct axk_bootparams_acpi_t
{
    uint32_t size;
    bool new_version;
    uint8_t data[ 48 ];
};

/*
    axk_x86_bootparams_parse
    * Internal Function (x86-64 only)
    * Parses information passed to the kernel from the T-0 bootloader
*/
bool axk_x86_bootparams_parse( void* ptr_params );

/*
    axk_bootparams_get_acpi
    * Internal Function
    * Gets a pointer to the ACPI info found when parsing the boot parameters
*/
const struct axk_bootparams_acpi_t* axk_bootparams_get_acpi( void );

#endif
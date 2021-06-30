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
    ax_bootparams_parse
    * Internal Function
    * This is the X86_64 specific version of this function
    * Parses multiboot info and fills the boot parameter structures (memorymap, framebuffer, acpi, etc..)
*/
void ax_bootparams_parse( void* ptr_multiboot );

/*
    axk_bootparams_get_acpi
    * Internal Function
    * Gets a pointer to the ACPI info found when parsing bootparameters (multiboot2 in the case of X86)
*/
const struct axk_bootparams_acpi_t* axk_bootparams_get_acpi( void );

#endif
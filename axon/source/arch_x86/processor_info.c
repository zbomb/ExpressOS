/*==============================================================
    Axon Kernel - Processor Info Functions (x86)
    2021, Zachary Berry
    axon/source/system/processor_info.c
==============================================================*/

#ifdef _X86_64_
#include "axon/system/processor_info.h"
#include "axon/arch_x86/util.h"
#include "axon/arch_x86/acpi_info.h"


/*
    Function Implementations
*/
uint32_t axk_processor_get_id( void )
{
    uint32_t eax, ebx, ecx, edx;
    eax = 0x1F;
    if( axk_x86_cpuid_s( &eax, &ebx, &ecx, &edx ) )
    {
        return edx;
    }
    else
    {
        eax = 0x0B; ebx = 0x00; ecx = 0x00; edx = 0x00;
        axk_x86_cpuid( &eax, &ebx, &ecx, &edx );
        return edx;
    }
}


uint32_t axk_processor_get_count( void )
{
    struct axk_x86_acpi_info_t* ptr_acpi = axk_x86_acpi_get();
    return ptr_acpi == NULL ? 1 : ptr_acpi->cpu_count;
}


const char* axk_processor_get_vendor( void )
{
    struct axk_x86_acpi_info_t* ptr_acpi = axk_x86_acpi_get();
    return ptr_acpi == NULL ? NULL : ptr_acpi->cpu_vendor;
}


uint32_t axk_processor_get_boot_id( void )
{
    struct axk_x86_acpi_info_t* ptr_acpi = axk_x86_acpi_get();
    return ptr_acpi == NULL ? 0 : ptr_acpi->bsp_id;
}

#endif
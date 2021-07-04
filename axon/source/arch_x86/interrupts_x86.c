/*==============================================================
    Axon Kernel - X86 Interrupts
    2021, Zachary Berry
    axon/source/arch_x86/interrupts_x86.c
==============================================================*/

#ifdef _X86_64_
#include "axon/system/interrupts_mgr.h"
#include "axon/arch_x86/drivers/xapic_driver.h"
#include "axon/arch_x86/util.h"
#include "axon/debug_print.h"
#include "axon/panic.h"

/*
    State
*/
static struct axk_interrupt_driver_t* g_int_driver;
static enum axk_interrupt_driver_type g_int_type;


/*
    Functions
*/
bool axk_interrupts_init( void )
{
    // Initialize global state
    axk_interrupts_init_state();
    
    // Determine the interrupt driver(s) we support on this system
    uint32_t eax, ebx, ecx, edx;
    eax = 0x01; 
    axk_x86_cpuid( &eax, &ebx, &ecx, &edx );

    if( ( ecx & ( 1U << 21U ) ) != 0U )
    {
        // TODO: Use X2APIC driver
        g_int_driver    = axk_x86_create_xapic_driver();
        g_int_type      = INTERRUPT_DRIVER_X86_X2APIC;
    }
    else
    {
        // No X2APIC support, check for XAPIC
        if( ( edx & ( 1U << 9U ) ) != 0U )
        {
            // Use XAPIC driver
            g_int_driver    = axk_x86_create_xapic_driver();
            g_int_type      = INTERRUPT_DRIVER_X86_XAPIC;
        }
        else
        {
            // No APIC support on this system!
            return false;
        }
    }

    // Initialize the driver..
    if( g_int_driver->init( g_int_driver ) )
    {
        // Print debug message
        axk_terminal_prints( "Interrupts: Initialized driver \"" );
        axk_terminal_prints( g_int_type == INTERRUPT_DRIVER_X86_XAPIC ? "xAPIC" : "x2APIC" );
        axk_terminal_prints( " (x86)\" \n" );
    }
    else
    {
        return false;
    }

    return true;
}


bool axk_interrupts_init_aux( void )
{
    if( g_int_driver == NULL )
    {
        axk_panic( "x86: Attempt to initialize interrupts on AP, but the interrupt driver was null" );
    }

    if( g_int_driver->aux_init( g_int_driver ) )
    {
        axk_terminal_lock();
        axk_terminal_prints( "Interrupts: Initialized interrupts on AP (x86) \n" );
        axk_terminal_unlock();
    }
    else
    {
        return false;
    }

    return true;
}


struct axk_interrupt_driver_t* axk_interrupts_get( void )
{
    return g_int_driver;
}


enum axk_interrupt_driver_type axk_interrupts_get_type( void )
{
    return g_int_type;
}

#endif
/*==============================================================
    Axon Kernel - X86 Timers
    2021, Zachary Berry
    axon/source/arch_x86/timers_x86.c
==============================================================*/

#include "axon/system/timers_mgr.h"
#include "axon/arch_x86/acpi_info.h"
#include "axon/arch_x86/util.h"
#include "axon/arch_x86/drivers/tsc_driver.h"
#include "axon/arch_x86/drivers/pit_driver.h"
#include "axon/arch_x86/drivers/hpet_driver.h"
#include "axon/arch_x86/drivers/acpi_timer_driver.h"
#include "axon/arch_x86/drivers/lapic_timer_driver.h"
#include "stdlib.h"
#include "axon/debug_print.h"


/*
    State
*/
struct axk_timer_driver_t* g_timers[ 5 ];
uint32_t g_timer_count;
struct axk_timer_driver_t* g_local_timer;
struct axk_timer_driver_t* g_external_timer;
struct axk_timer_driver_t* g_counter;


/*
    Functions
*/
bool axk_timers_init( void )
{
    // Guard against double-init
    if( g_timer_count > 0 ) { return false; }

    struct axk_timer_driver_t* ptr_hpet     = NULL;
    struct axk_timer_driver_t* ptr_pit      = NULL;
    struct axk_timer_driver_t* ptr_lapic    = NULL;
    struct axk_timer_driver_t* ptr_tsc      = NULL;
    struct axk_timer_driver_t* ptr_acpi_pm  = NULL;

    struct axk_x86_acpi_info_t* ptr_acpi = axk_x86_acpi_get();

    // Create HPET Driver
    if( ptr_acpi->hpet_info != NULL )
    {
        ptr_hpet = axk_x86_create_hpet_driver( ptr_acpi->hpet_info );
        if( ptr_hpet != NULL )
        {
            if( ptr_hpet->init( ptr_hpet ) )
            {
                g_timers[ g_timer_count++ ] = ptr_hpet;
            }
            else
            {
                axk_terminal_prints( "Timers (x86): Failed to initialize HPET driver\n" );
                free( ptr_hpet );
                ptr_hpet = NULL;
            }
        }
    }

    // Create PIT Driver
    if( ptr_hpet == NULL || !ptr_acpi->hpet_info->is_legacy_replacment )
    {
        ptr_pit = axk_x86_create_pit_driver();
        if( ptr_pit != NULL )
        {
            if( ptr_pit->init( ptr_pit ) )
            {
                g_timers[ g_timer_count++ ] = ptr_pit;
            }
            else
            {
                free( ptr_pit );
                ptr_pit = NULL;
            }
        }

        if( ptr_pit == NULL )
        {
            axk_terminal_prints( "Timers (x86): Failed to create PIT driver!\n" );
            if( ptr_hpet != NULL ) { free( ptr_hpet ); }
            return false;
        }
    }

    // Create Invariant TSC Driver
    uint32_t eax, ebx, ecx, edx;
    eax = 0x80000007U; ebx = 0U; ecx = 0U; edx = 0U;
    axk_x86_cpuid( &eax, &ebx, &ecx, &edx );

    if( ( edx & ( 1U << 8U ) ) != 0U )
    {
        // Create TSC driver
        ptr_tsc = axk_x86_create_tsc_driver();
        if( ptr_tsc != NULL )
        {
            if( ptr_tsc->init( ptr_tsc ) )
            {
                g_timers[ g_timer_count++ ] = ptr_tsc;
            }
            else
            {
                axk_terminal_prints( "Timers (x86): Failed to initialize Invariant TSC driver\n" );
                free( ptr_tsc );
                ptr_tsc = NULL;
            }
        }
    }

    // Create Local-APIC Timer
    ptr_lapic = axk_x86_create_lapic_timer_driver();
    if( ptr_lapic != NULL )
    {
        if( ptr_lapic->init( ptr_lapic ) )
        {
            g_timers[ g_timer_count++ ] = ptr_lapic;
        }
        else
        {
            free( ptr_lapic );
            ptr_lapic = NULL;
        }
    }

    if( ptr_lapic == NULL )
    {
        axk_terminal_prints( "Timers (x86): Failed to create Local-APIC timer!\n" );
        for( uint32_t i = 0; i < g_timer_count; i++ ) { free( g_timers[ i ] ); }
        return false;
    }

    // TODO: Create ACPI PM Timer

    // Now, lets select the optimal timer sources for each purpose (local, external & counter)
    // Were always going to select the local APIC timer for the local source
    g_local_timer = ptr_lapic;

    // For the external timer, were going to prefer HPET, then ACPI PM, then PIT
    if( ptr_hpet != NULL )
    {
        g_external_timer = ptr_hpet;
    }
    else if( ptr_acpi_pm != NULL )
    {
        g_external_timer = ptr_acpi_pm;
    }
    else
    {
        g_external_timer = ptr_pit;
    }

    // For the counter, were going to prefer TSC, and then HPET
    if( ptr_tsc != NULL )
    {
        g_counter = ptr_tsc;
    }
    else if( ptr_hpet != NULL )
    {
        // TODO: Should we allow HPET to be used for counter and external timing sources
        g_counter = ptr_hpet;
    }
    else
    {
        axk_terminal_prints( "Timers (x86): There is no high-precision counter source available\n" );
        for( uint32_t i = 0; i < g_timer_count; i++ ) { free( g_timers[ i ] ); }
        return false;
    }

    // Now, lets calibrate the timer sources that require it
    if( g_counter == ptr_tsc && !axk_x86_calibrate_tsc_driver( ptr_tsc ) )
    {
        axk_terminal_prints( "Timers (x86): Counter calibration failed\n" );
        for( uint32_t i = 0; i < g_timer_count; i++ ) { free( g_timers[ i ] ); }
        return false;
    }

    if( !axk_x86_calibrate_lapic_timer( ptr_lapic ) )
    {
        axk_terminal_prints( "Timers (x86): Local-APIC Timer calibration failed!\n" );
        for( uint32_t i = 0; i < g_timer_count; i++ ) { free( g_timers[ i ] ); }
        return false;
    }

    // And lets print out the debug message
    axk_terminal_prints( "Timers: Initialized " );
    axk_terminal_printu32( g_timer_count );
    axk_terminal_prints( " timer driver(s). \n\tSources: Ext=" );

    if( g_external_timer == ptr_hpet )          { axk_terminal_prints( "HPET" ); }
    else if( g_external_timer == ptr_acpi_pm )  { axk_terminal_prints( "ACPI PM" ); }
    else if( g_external_timer == ptr_pit )      { axk_terminal_prints( "PIT" ); }

    axk_terminal_prints( "  Local=LAPIC  Counter=" );

    if( g_counter == ptr_tsc )          { axk_terminal_prints( "InvTSC" ); }
    else if( g_counter == ptr_hpet )    { axk_terminal_prints( "HPET" ); }

    axk_terminal_printnl();

    return true;
}


uint32_t axk_timer_get_count( void )
{
    return g_timer_count;
}


struct axk_timer_driver_t* axk_timer_get( uint32_t index )
{
    if( index >= g_timer_count ) { return NULL; }
    return g_timers[ index ];
}


struct axk_timer_driver_t* axk_timer_get_local( void )
{
    return g_local_timer;
}


struct axk_timer_driver_t* axk_timer_get_external( void )
{
    return g_external_timer;
}


struct axk_timer_driver_t* axk_timer_get_counter( void )
{
    return g_counter;
}

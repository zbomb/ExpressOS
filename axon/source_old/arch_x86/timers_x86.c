/*==============================================================
    Axon Kernel - X86 Timers
    2021, Zachary Berry
    axon/source/arch_x86/timers_x86.c
==============================================================*/

#include "axon/system/timers_private.h"
#include "axon/arch_x86/acpi_info.h"
#include "axon/arch_x86/util.h"
#include "axon/arch_x86/drivers/tsc_driver.h"
#include "axon/arch_x86/drivers/pit_driver.h"
#include "axon/arch_x86/drivers/hpet_driver.h"
#include "axon/arch_x86/drivers/acpi_timer_driver.h"
#include "axon/arch_x86/drivers/lapic_timer_driver.h"
#include "axon/boot/basic_out.h"
#include "axon/panic.h"
#include "axon/memory/atomics.h"
#include "axon/system/time.h"
#include "axon/system/time_private.h"
#include "axon/arch_x86/acpi_info.h"
#include "axon/arch.h"
#include "stdlib.h"
#include "big_math.h"


/*
    State
*/
static struct axk_timer_driver_t* g_timers[ 5 ];
static uint32_t g_timer_count = 0;
static struct axk_timer_driver_t* g_local_timer = NULL;
static struct axk_timer_driver_t* g_external_timer = NULL;
static struct axk_timer_driver_t* g_counter  = NULL;
static uint64_t* g_tsc_sync_table = NULL;
static struct axk_atomic_uint32_t g_tsc_sync_point;
static struct axk_atomic_bool_t g_tsc_sync_point_2;
static struct axk_atomic_uint32_t g_tsc_sync_point_3;


/*
    Functions
*/
bool axk_timers_init( void )
{
    // Guard against double-init
    if( g_timer_count > 0 ) { return false; }

    axk_atomic_store_uint32( &g_tsc_sync_point, 0, MEMORY_ORDER_SEQ_CST );
    axk_atomic_store_bool( &g_tsc_sync_point_2, false, MEMORY_ORDER_SEQ_CST );

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
                axk_basicout_prints( "Timers (x86): Failed to initialize HPET driver\n" );
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
            axk_basicout_prints( "Timers (x86): Failed to create PIT driver!\n" );
            if( ptr_hpet != NULL ) { free( ptr_hpet ); }
            return false;
        }
    }

    // Create Invariant TSC Driver
    uint32_t eax, ebx, ecx, edx;
    eax = 0x80000007U; ebx = 0U; ecx = 0U; edx = 0U;
    if( axk_x86_cpuid_s( &eax, &ebx, &ecx, &edx ) )
    {
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
                    axk_basicout_prints( "Timers (x86): Failed to initialize Invariant TSC driver\n" );
                    free( ptr_tsc );
                    ptr_tsc = NULL;
                }
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
        axk_basicout_prints( "Timers (x86): Failed to create Local-APIC timer!\n" );
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
        axk_basicout_prints( "Timers (x86): There is no high-precision counter source available\n" );
        for( uint32_t i = 0; i < g_timer_count; i++ ) { free( g_timers[ i ] ); }
        return false;
    }

    // Now, lets calibrate the timer sources that require it
    if( g_counter == ptr_tsc && !axk_x86_calibrate_tsc_driver( ptr_tsc ) )
    {
        axk_basicout_prints( "Timers (x86): Counter calibration failed\n" );
        for( uint32_t i = 0; i < g_timer_count; i++ ) { free( g_timers[ i ] ); }
        return false;
    }

    if( !axk_x86_calibrate_lapic_timer( ptr_lapic ) )
    {
        axk_basicout_prints( "Timers (x86): Local-APIC Timer calibration failed!\n" );
        for( uint32_t i = 0; i < g_timer_count; i++ ) { free( g_timers[ i ] ); }
        return false;
    }

    // And lets print out the debug message
    axk_basicout_prints( "Timers: Initialized " );
    axk_basicout_printu32( g_timer_count );
    axk_basicout_prints( " timer driver(s). \n\tSources: Ext=" );

    if( g_external_timer == ptr_hpet )          { axk_basicout_prints( "HPET" ); }
    else if( g_external_timer == ptr_acpi_pm )  { axk_basicout_prints( "ACPI PM" ); }
    else if( g_external_timer == ptr_pit )      { axk_basicout_prints( "PIT" ); }

    axk_basicout_prints( "  Local=LAPIC  Counter=" );

    if( g_counter == ptr_tsc )          { axk_basicout_prints( "InvTSC" ); }
    else if( g_counter == ptr_hpet )    { axk_basicout_prints( "HPET" ); }

    axk_basicout_printnl();

    return true;
}


bool axk_timers_bsp_sync( void )
{
    // Check if we have to do a TSC Sync
    struct axk_x86_acpi_info_t* ptr_acpi = axk_x86_acpi_get();
    if( ptr_acpi == NULL ) { return false; }

    uint32_t proc_id        = axk_get_cpu_id();
    uint32_t proc_count     = ptr_acpi->lapic_count;
    uint32_t sync_count     = proc_count - 1u;
    bool b_require_unlock   = false;

    if( g_counter->get_id() == AXK_TIMER_ID_TSC )
    {
        if( proc_count > 1 )
        {
            // Allocate a table we can write each processors TSC value into
            g_tsc_sync_table        = calloc( proc_count, sizeof( uint64_t ) );
            g_tsc_sync_table[ 0 ]   = axk_x86_read_timestamp();

            // All core sync point
            while( axk_atomic_load_uint32( &g_tsc_sync_point, MEMORY_ORDER_SEQ_CST ) < sync_count ) { __asm__( "pause" ); }
            axk_atomic_store_uint32( &g_tsc_sync_point, 0U, MEMORY_ORDER_SEQ_CST );

            // Update counter value
            axk_x86_write_msr( AXK_X86_MSR_TSC, g_tsc_sync_table[ 0 ] );
            
            // Now, we need to wait for a small amount of time for the timestamp counter to increase
            axk_delay( 100000000UL );

            // Release the other processors to write their TSC values into the array
            axk_atomic_store_bool( &g_tsc_sync_point_2, true, MEMORY_ORDER_SEQ_CST );
            g_tsc_sync_table[ proc_id ] = axk_x86_read_timestamp();

            // Wait for the cores to complete this
            while( axk_atomic_load_uint32( &g_tsc_sync_point_3, MEMORY_ORDER_SEQ_CST ) < sync_count ) { __asm__( "pause" ); }
            b_require_unlock = true;

            // And now were going to do the math
            uint64_t min_value = 0xFFFFFFFFFFFFFFFFUL;
            uint64_t max_value = 0x0000000000000000UL;

            for( uint32_t i = 0; i < proc_count; i++ )
            {
                if( g_tsc_sync_table[ i ] > max_value ) { max_value = g_tsc_sync_table[ i ]; }
                if( g_tsc_sync_table[ i ] < min_value ) { min_value = g_tsc_sync_table[ i ]; }
            }

            free( g_tsc_sync_table );
            g_tsc_sync_table = NULL;

            uint64_t avg_value      = ( min_value + max_value ) / 2UL;
            uint64_t diff_value     = ( ( max_value - min_value ) * 10000UL ) / avg_value;

            if( diff_value > 0UL )
            {
                // Since TSC is not reliable here, we need to fallback on another counter source.. i.e. HPET
                struct axk_timer_driver_t* ptr_hpet = axk_timer_get_by_id( AXK_TIMER_ID_HPET );
                if( ptr_hpet == NULL )
                {
                    axk_panic( "Timers (x86): TSC syncronization failed, and there was no other counter source to fallback on" );
                }

                axk_basicout_lock();
                axk_basicout_prints( "Timers (x86): TSC syncronization failed.. falling back to use HPET\n" );
                axk_basicout_unlock();

                g_counter = ptr_hpet;
                // TODO: Any sync needed or init needed for HPET use as a counter?
            }
        }
    }

    // Initialize the time keeping system
    axk_time_init( 50000000UL );

    uint32_t ext_id = axk_timer_get_id( g_external_timer );
    uint32_t result;
    switch( ext_id )
    {
        case AXK_TIMER_ID_HPET:
        result = axk_timer_start( g_external_timer, AXK_TIMER_MODE_PERIODIC, 50000000UL, false, ptr_acpi->bsp_id, AXK_INT_EXT_CLOCK_TICK );
        break;

        case AXK_TIMER_ID_PIT:
        result = axk_timer_start( g_external_timer, AXK_TIMER_MODE_DIVISOR, 59659UL, true, ptr_acpi->bsp_id, AXK_INT_EXT_CLOCK_TICK );
        break;
    }

    if( result != AXK_TIMER_ERROR_NONE )
    {
        axk_panic( "Timers (x86): Failed to start external timer used to keep track of system time" );
    }

    // Wait for syncronization to occur before continuing...
    axk_time_wait_for_sync();

    // And perform final thread release if needed
    if( b_require_unlock )
    {
        axk_atomic_store_uint32( &g_tsc_sync_point_3, 0U, MEMORY_ORDER_SEQ_CST );
    }

    // Print debug message
    axk_basicout_prints( "System Clock: Syncronized processors! \n" );

    return true;
}


bool axk_timers_ap_sync( void )
{    
    // Determine if we need to perform TSC syncronization
    if( g_counter->get_id() == AXK_TIMER_ID_TSC )
    {
        uint32_t proc_id = axk_get_cpu_id();

        // Increment lock and wait for it to be reset
        axk_atomic_fetch_add_uint32( &g_tsc_sync_point, 1, MEMORY_ORDER_SEQ_CST );
        while( axk_atomic_load_uint32( &g_tsc_sync_point, MEMORY_ORDER_SEQ_CST ) > 0 ) { __asm__( "pause" ); }

        // Set the local timestamp counter
        axk_x86_write_msr( AXK_X86_MSR_TSC, g_tsc_sync_table[ 0 ] );

        // Sync again..
        while( !axk_atomic_load_bool( &g_tsc_sync_point_2, MEMORY_ORDER_SEQ_CST ) ) { __asm__( "pause" ); }

        // Write new timestamp counter value into the array
        g_tsc_sync_table[ proc_id ] = axk_x86_read_timestamp();
    }

    // And wait for the final sync point
    axk_atomic_fetch_add_uint32( &g_tsc_sync_point_3, 1, MEMORY_ORDER_ACQUIRE );
    while( axk_atomic_load_uint32( &g_tsc_sync_point_3, MEMORY_ORDER_ACQUIRE ) > 0 ) { __asm__( "pause" ); }

    return true;
}


void axk_delay( uint64_t in_nano )
{
    if( in_nano < 10UL ) { return; }
    if( g_counter == NULL ) { axk_panic( "Time (x86): Attempt to call 'delay', but timers havent been initialized" ); }
    
    uint64_t start_value    = g_counter->get_counter( g_counter );
    uint64_t counter_freq   = g_counter->get_frequency( g_counter );
    
    uint64_t delta = ( counter_freq > ( 0xFFFFFFFFFFFFFFFFUL / in_nano ) ) ?
        _muldiv64( counter_freq, in_nano, 1000000000UL ) :
        ( counter_freq * in_nano ) / 1000000000UL;
    
    // We also want to account for overflow
    uint64_t counter_max = g_counter->get_max_value( g_counter );
    uint64_t target_value;

    if( delta > ( counter_max - start_value ) )
    {
        target_value = delta - ( counter_max - start_value );
        if( target_value > counter_max )
        {
            axk_panic( "Timers (x86): Delay failed, the target delay would cause the counter to wrap around" );
        }

        // Wait for the overflow to occur
        while( g_counter->get_counter( g_counter ) >= start_value ) { __asm__( "pause" ); }
    }
    else
    {
        target_value = start_value + delta;
    }

    // Wait to hit final target value
    while( g_counter->get_counter( g_counter ) < target_value ) { __asm__( "pause" ); }
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

/*==============================================================
    Axon Kernel - x86 TSC Driver
    2021, Zachary Berry
    axon/source/arch_x86/tsc_driver.c
==============================================================*/

#include "axon/arch_x86/drivers/tsc_driver.h"
#include "axon/arch_x86/util.h"
#include "axon/boot/basic_out.h"
#include "axon/system/interrupts.h"
#include "axon/arch.h"
#include "stdlib.h"

/*
    Create Driver Function
*/
bool tsc_init( struct axk_timer_driver_t* self );
bool tsc_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats );
uint32_t tsc_get_id( void );
uint64_t tsc_get_frequency( struct axk_timer_driver_t* self );
uint32_t tsc_start( struct axk_timer_driver_t* self, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, uint32_t processor, uint8_t vector );
bool tsc_stop( struct axk_timer_driver_t* self );
bool tsc_is_running( struct axk_timer_driver_t* self );
uint64_t tsc_get_counter( struct axk_timer_driver_t* self );
uint64_t tsc_get_max_value( struct axk_timer_driver_t* self );



struct axk_timer_driver_t* axk_x86_create_tsc_driver( void )
{
    struct axk_x86_tsc_driver_t* output     = AXK_CALLOC_TYPE( struct axk_x86_tsc_driver_t, 1 );
    output->func_table.init                 = tsc_init;
    output->func_table.query_features       = tsc_query_features;
    output->func_table.get_id               = tsc_get_id;
    output->func_table.get_frequency        = tsc_get_frequency;
    output->func_table.start                = tsc_start;
    output->func_table.stop                 = tsc_stop;
    output->func_table.is_running           = tsc_is_running;
    output->func_table.get_counter          = tsc_get_counter;
    output->func_table.get_max_value        = tsc_get_max_value;

    return (struct axk_timer_driver_t*)( output );
}

/*
    Calibrate Driver Function
*/
static uint32_t calibrate_ticks;
static uint64_t calibrate_value;
static uint32_t calibrate_target;

static bool calibrate_callback( uint8_t _unused_ )
{
    if( calibrate_ticks == 1 )
    {
        // Read the starting value
        calibrate_value = axk_x86_read_timestamp();
    }
    else if( calibrate_ticks == calibrate_target )
    {
        // Calculate the delta since the first callback and stop the timer
        calibrate_value = axk_x86_read_timestamp() - calibrate_value;
        axk_timer_stop( axk_timer_get_external() );
    }
    else if( calibrate_ticks > calibrate_target )
    {
        axk_timer_stop( axk_timer_get_external() );
    }

    calibrate_ticks++;
    return false;
}

bool axk_x86_calibrate_tsc_driver( struct axk_timer_driver_t* self )
{
    struct axk_x86_tsc_driver_t* this = (struct axk_x86_tsc_driver_t*)( self );
    if( this == NULL ) { return false; }

    uint64_t cpuid_tsc_frequency = 0UL;

    uint32_t eax, ebx, ecx, edx;
    eax = 0x15; ebx = 0x00; ecx = 0x00; edx = 0x00;
    if( axk_x86_cpuid_s( &eax, &ebx, &ecx, &edx ) )
    {
        if( ecx != 0x0 && ebx != 0x00 && eax != 0x00 )
        {
            // We can get the TSC frequency from CPUID!
            cpuid_tsc_frequency = (uint64_t)( ecx ) * ( (uint64_t)( ebx ) / (uint64_t)( eax ) );
        }
    }

    // We need to grab another timer, that is already calibrated and use it to calculate the rate the counter is increasing
    struct axk_timer_driver_t* ptr_timer = axk_timer_get_external();
    if( ptr_timer == NULL ) { return false; }

    // Start calibration timer
    calibrate_ticks = 0;
    calibrate_value = 0;

    uint32_t processor = axk_get_cpu_id();
    uint8_t int_vec;

    if( !axk_interrupts_acquire_handler( AXK_PROCESS_KERNEL, calibrate_callback, &int_vec ) )
    {
        return false;
    }

    // Determine if were using PIT, or if were using HPET to calibrate
    if( ptr_timer->get_id() == AXK_TIMER_ID_PIT )
    {
        // In this case, we need to select a divisor, and we want to run for a total of 0.5s, so, to achieve ~0.05s per tick, the divisor should be 59659
        calibrate_target = 21;
        uint32_t result = axk_timer_start( ptr_timer, AXK_TIMER_MODE_DIVISOR, 59659, false, processor, int_vec );
        if( result != AXK_TIMER_ERROR_NONE ) { return false; }
    }
    else
    {
        // In this case, we are going to use 0.25s ticks, still targetting 0.5s delta total
        calibrate_target = 5;
        uint32_t result = axk_timer_start( ptr_timer, AXK_TIMER_MODE_PERIODIC, 250000000, false, processor, int_vec );
        if( result != AXK_TIMER_ERROR_NONE ) { return false; }
    }

    // Now, we are going to wait for calibration to complete
    
    while( calibrate_ticks <= calibrate_target ) { __asm__( "pause" ); }

    if( cpuid_tsc_frequency > 0UL )
    {
        // Compare against the frequency we observed
        uint64_t observed = calibrate_value;
        uint64_t diff = observed > cpuid_tsc_frequency ? observed - cpuid_tsc_frequency : cpuid_tsc_frequency - observed;

        if( diff > observed / 100UL ) 
        {
            this->frequency = observed;

            axk_basicout_prints( "TSC (x86): Warning, the observed TSC frequency varies from the rate calculated from CPUID\n" );
            axk_basicout_prints( "\t\t Observed: " );
            axk_basicout_printu64( observed );
            axk_basicout_prints( " Hz   CPUID: " );
            axk_basicout_printu64( cpuid_tsc_frequency );
            axk_basicout_prints( " Hz, going to use the observed frequency\n" );
        }
        else
        {
            this->frequency = cpuid_tsc_frequency;

            axk_basicout_prints( "TSC (x86): Acquired TSC rate from CPUID, running at " );
            axk_basicout_printu64( cpuid_tsc_frequency );
            axk_basicout_prints( " Hz \n" );
        }
    }
    else
    {
        this->frequency = calibrate_value;

        axk_basicout_prints( "TSC (x86): Using observed TSC rate, running at approx. " );
        axk_basicout_printu64( this->frequency );
        axk_basicout_prints( " Hz \n" );
    }

    axk_interrupts_release_handler( int_vec );
    return true;
}


/*
    Driver Functions
*/
bool tsc_init( struct axk_timer_driver_t* self )
{
    return true;
}


bool tsc_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats )
{
    return AXK_TIMER_FEATURE_COUNTER | AXK_TIMER_FEATURE_INVARIANT;
}


uint32_t tsc_get_id( void )
{
    return AXK_TIMER_ID_TSC;
}


uint64_t tsc_get_frequency( struct axk_timer_driver_t* self )
{
    struct axk_x86_tsc_driver_t* this = (struct axk_x86_tsc_driver_t*)( self );
    return this->frequency;
}


uint32_t tsc_start( struct axk_timer_driver_t* self, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, uint32_t processor, uint8_t vector )
{
    return AXK_TIMER_ERROR_COUNTER_ONLY;
}


bool tsc_stop( struct axk_timer_driver_t* self )
{
    return false;
}


bool tsc_is_running( struct axk_timer_driver_t* self )
{
    return false;
}


uint64_t tsc_get_counter( struct axk_timer_driver_t* self )
{
    return axk_x86_read_timestamp();
}


uint64_t tsc_get_max_value( struct axk_timer_driver_t* self )
{
    return 0xFFFFFFFFFFFFFFFFUL;
}
/*==============================================================
    Axon Kernel - x86 PIT Driver
    2021, Zachary Berry
    axon/source/arch_x86/pit_driver.c
==============================================================*/

#include "axon/arch_x86/drivers/pit_driver.h"
#include "axon/system/interrupts.h"
#include "axon/arch.h"
#include "axon/arch_x86/util.h"
#include "axon/debug_print.h"
#include "stdlib.h"
#include "big_math.h"

/*
    Constants
*/
#define PIT_PORT_MODE_COMMAND   0x43
#define PIT_PORT_CHANNEL_0      0x40
#define PIT_PORT_CHANNEL_1      0x41
#define PIT_PORT_CHANNEL_2      0x42
#define PIT_FREQUENCY           1193182UL

/*
    Create Driver Function
*/
bool pit_init( struct axk_timer_driver_t* self );
bool pit_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats );
uint32_t pit_get_id( void );
uint64_t pit_get_frequency( struct axk_timer_driver_t* self );
uint32_t pit_start( struct axk_timer_driver_t* self, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, void( *callback )( void ) );
bool pit_stop( struct axk_timer_driver_t* self );
bool pit_is_running( struct axk_timer_driver_t* self );
uint64_t pit_get_counter( struct axk_timer_driver_t* self );
uint64_t pit_get_max_value( struct axk_timer_driver_t* self );


struct axk_timer_driver_t* axk_x86_create_pit_driver( void )
{
    struct axk_x86_pit_driver_t* output = AXK_CALLOC_TYPE( struct axk_x86_pit_driver_t, 1 );

    output->func_table.init             = pit_init;
    output->func_table.query_features   = pit_query_features;
    output->func_table.get_id           = pit_get_id;
    output->func_table.get_frequency    = pit_get_frequency;
    output->func_table.start            = pit_start;
    output->func_table.stop             = pit_stop;
    output->func_table.is_running       = pit_is_running;
    output->func_table.get_counter      = pit_get_counter;
    output->func_table.get_max_value    = pit_get_max_value;

    return (struct axk_timer_driver_t*)( output );
}


/*
    Driver Functions
*/
// TODO: Is there anyway to bind a state pointer to the main callback function?
// This would allow us to store this function pointer in the state, which seems more.. clean
static void( *g_callback )( void );

bool pit_callback( uint8_t int_vec )
{
    if( g_callback != NULL )
    {
        g_callback();
    }

    return false;
}


bool pit_init( struct axk_timer_driver_t* self )
{
    struct axk_x86_pit_driver_t* this = (struct axk_x86_pit_driver_t*)( self );
    if( this == NULL ) { return false; }

    // Determine the global interrupt were wired to
    this->global_interrupt = axk_interrupts_get_ext_number( 0, 0 );

    // Setup state
    g_callback = NULL;
    axk_atomic_clear_flag( &( this->b_running ), MEMORY_ORDER_SEQ_CST );

    // Acquire local interrupt handler
    uint8_t local_interrupt = 0;
    if( !axk_interrupts_acquire_handler( AXK_PROCESS_KERNEL, pit_callback, &local_interrupt ) )
    {
        return false;
    }

    // Setup the global interrupt mapping, route to this processor for now
    struct axk_external_interrupt_routing_t ext_routing;

    ext_routing.global_interrupt    = this->global_interrupt;
    ext_routing.local_interrupt     = local_interrupt;
    ext_routing.b_low_priority      = false;
    ext_routing.b_active_low        = false;
    ext_routing.b_level_triggered   = false;
    ext_routing.b_masked            = false;
    ext_routing.target_processor    = axk_get_processor_id();

    if( !axk_interrupts_set_ext_routing( &ext_routing ) )
    {
        return false;
    }

    // Disable interrupts and setup the timer
    uint64_t rflags = axk_disable_interrupts();

    axk_x86_outb( PIT_PORT_MODE_COMMAND, 0b00110000 );
    axk_x86_outb( PIT_PORT_CHANNEL_0, 0x00 );
    axk_x86_outb( PIT_PORT_CHANNEL_0, 0x00 );
    axk_interrupts_signal_eoi();

    axk_restore_interrupts( rflags );
    return true;
}


bool pit_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats )
{
    return AXK_CHECK_FLAG( 
        AXK_TIMER_FEATURE_EXTERNAL | AXK_TIMER_FEATURE_COUNTER | AXK_TIMER_FEATURE_ONE_SHOT | AXK_TIMER_FEATURE_INVARIANT | AXK_TIMER_FEATURE_DIVISOR,
        feats
    );
}


uint32_t pit_get_id( void )
{
    return AXK_TIMER_ID_PIT;
}


uint64_t pit_get_frequency( struct axk_timer_driver_t* self )
{
    // Fixed frequency, @ approx 1.1931816666 MHz
    return PIT_FREQUENCY;
}


uint32_t pit_start( struct axk_timer_driver_t* self, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, void( *callback )( void ) )
{
    // Validate parameters
    if( self == NULL || delay == 0UL || callback == NULL ) { return AXK_TIMER_ERROR_INVALID_PARAMS; }
    if( mode != AXK_TIMER_MODE_ONE_SHOT && mode != AXK_TIMER_MODE_DIVISOR ) { return AXK_TIMER_ERROR_INVALID_MODE; }

    struct axk_x86_pit_driver_t* this = (struct axk_x86_pit_driver_t*)( self );

    // If were already running, then we wont start
    uint64_t rflags = axk_disable_interrupts();
    if( axk_atomic_test_and_set_flag( &( this->b_running ), MEMORY_ORDER_SEQ_CST ) )
    {
        axk_restore_interrupts( rflags );
        return AXK_TIMER_ERROR_ALREADY_RUNNING;
    }

    // Start settings everything up...
    g_callback = callback;

    if( mode == AXK_TIMER_MODE_ONE_SHOT )
    {
        // We need to convert the source time (in nanoseconds), to PIT ticks [( Frequency(Hz) * 1e9 ) / Time(nS)]
        uint64_t ticks = b_delay_in_ticks ? delay : _muldiv64( PIT_FREQUENCY, 1000000000UL, delay );

        // Check if 'ticks' exceeds the max counter value, since the PIT counter is only 16-bits wide
        if( ticks > 0xFFFFUL )
        {
            axk_terminal_prints( "PIT (x86): [Warning] Attempt to use 'one-shot' mode, but delay exceeds the max allowed delay due to hardware limitations (54.9 ms or 65535 ticks)\n" );
            axk_restore_interrupts( rflags );
            return AXK_TIMER_ERROR_INVALID_PARAMS;
        }

        axk_x86_outb( PIT_PORT_MODE_COMMAND, 0b00110000 );
        axk_x86_outb( PIT_PORT_CHANNEL_0, (uint8_t)( ticks & 0x00FFUL ) );
        axk_x86_outb( PIT_PORT_CHANNEL_0, (uint8_t)( ( ticks & 0xFF00UL ) >> 8 ) );
        axk_restore_interrupts( rflags );

        return AXK_TIMER_ERROR_NONE;
    }
    else // AXK_TIMER_MODE_DIVISOR
    {
        // Ensure the divisor is valid
        if( delay > 0xFFFFUL )
        {
            axk_terminal_prints( "PIT (x86): [Warning] Attempt to use 'divisor' mode, but the divisor is out of bounds (Max is 65535)\n" );
            axk_restore_interrupts( rflags );
            return AXK_TIMER_ERROR_INVALID_PARAMS;
        }

        axk_x86_outb( PIT_PORT_MODE_COMMAND, 0b00110100 );
        axk_x86_outb( PIT_PORT_CHANNEL_0, (uint8_t)( delay & 0xFFFFUL ) );
        axk_x86_outb( PIT_PORT_CHANNEL_0, (uint8_t)( ( delay & 0xFFUL ) >> 8 ) );
        axk_restore_interrupts( rflags );

        return AXK_TIMER_ERROR_NONE;
    }
}


bool pit_stop( struct axk_timer_driver_t* self )
{
    struct axk_x86_pit_driver_t* this = (struct axk_x86_pit_driver_t*)( self );
    if( this == NULL ) { return false; }

    // Ensure interrupts are disabled while we do this so we arent preempted
    uint64_t rflags = axk_disable_interrupts();

    axk_x86_outb( PIT_PORT_MODE_COMMAND, 0b00110000 );
    axk_atomic_clear_flag( &( this->b_running ), MEMORY_ORDER_SEQ_CST );

    axk_restore_interrupts( rflags );
    return true;
}


bool pit_is_running( struct axk_timer_driver_t* self )
{
    struct axk_x86_pit_driver_t* this = (struct axk_x86_pit_driver_t*)( self );
    if( this == NULL ) { return false; }

    return axk_atomic_test_flag( &( this->b_running ), MEMORY_ORDER_SEQ_CST );
}


uint64_t pit_get_counter( struct axk_timer_driver_t* self )
{
    // We dont allow the counter to be directly read
    return 0UL;
}


uint64_t pit_get_max_value( struct axk_timer_driver_t* self )
{
    return 0xFFFFUL;
}


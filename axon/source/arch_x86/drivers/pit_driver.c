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
uint32_t pit_start( struct axk_timer_driver_t* self, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, uint32_t processor, uint8_t vector );
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
bool pit_init( struct axk_timer_driver_t* self )
{
    struct axk_x86_pit_driver_t* this = (struct axk_x86_pit_driver_t*)( self );
    if( this == NULL ) { return false; }

    // Determine the global interrupt were wired to
    this->global_interrupt = axk_interrupts_get_ext_number( 0, 0 );

    // Setup the global interrupt mapping, route to this processor for now
    struct axk_external_interrupt_routing_t ext_routing;

    ext_routing.global_interrupt    = this->global_interrupt;
    ext_routing.local_interrupt     = AXK_INT_IGNORED;
    ext_routing.b_low_priority      = false;
    ext_routing.b_active_low        = false;
    ext_routing.b_level_triggered   = false;
    ext_routing.b_masked            = false;
    ext_routing.target_processor    = axk_get_cpu_id();

    if( !axk_interrupts_lock_external( AXK_PROCESS_KERNEL, &ext_routing, true ) )
    {
        return false;
    }

    this->target_processor = ext_routing.target_processor;
    this->target_interrupt = AXK_INT_IGNORED;

    // Disable interrupts and setup the timer 
    uint64_t rflags = axk_disable_interrupts();

    axk_x86_outb( PIT_PORT_MODE_COMMAND, 0b00110000 );
    axk_x86_waitio();
    axk_x86_outb( PIT_PORT_CHANNEL_0, 0x00 );
    axk_x86_waitio();
    axk_x86_outb( PIT_PORT_CHANNEL_0, 0x00 );
    axk_x86_waitio();
    axk_interrupts_signal_eoi();
    axk_restore_interrupts( rflags );

    axk_spinlock_init( &( this->lock ) );

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


uint32_t pit_start( struct axk_timer_driver_t* self, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, uint32_t processor, uint8_t vector )
{
    // Validate parameters
    if( self == NULL || delay == 0UL || vector < AXK_INT_MINIMUM ) { return AXK_TIMER_ERROR_INVALID_PARAMS; }
    if( mode != AXK_TIMER_MODE_ONE_SHOT && mode != AXK_TIMER_MODE_DIVISOR ) { return AXK_TIMER_ERROR_INVALID_MODE; }

    struct axk_x86_pit_driver_t* this = (struct axk_x86_pit_driver_t*)( self );

    // Acquire lock to have exclusive access to the hardware
    axk_spinlock_acquire( &( this->lock ) );

    // Update external interrupt routing
    if( processor != this->target_processor || vector != this->target_interrupt )
    {
        struct axk_external_interrupt_routing_t routing;

        routing.global_interrupt    = this->global_interrupt;
        routing.local_interrupt     = vector;
        routing.b_low_priority      = false;
        routing.b_active_low        = false;
        routing.b_level_triggered   = false;
        routing.b_masked            = false;
        routing.target_processor    = processor;

        if( !axk_interrupts_update_external( this->global_interrupt, &routing ) )
        {
            axk_spinlock_release( &( this->lock ) );
            return AXK_TIMER_ERROR_INVALID_PARAMS;
        }
    }

    if( mode == AXK_TIMER_MODE_ONE_SHOT )
    {
        // We need to convert the source time (in nanoseconds), to PIT ticks
        uint64_t ticks;
        if( b_delay_in_ticks )
        {
            ticks = delay;
        }
        else
        {
            ticks = ( PIT_FREQUENCY > ( 0xFFFFFFFFFFFFFFFFUL / delay ) ) ?
                _muldiv64( PIT_FREQUENCY, delay, 1000000000UL ) :
                ( PIT_FREQUENCY * delay ) / 1000000000UL;
        }

        // Check if 'ticks' exceeds the max counter value, since the PIT counter is only 16-bits wide
        if( ticks > 0xFFFFUL )
        {
            axk_terminal_prints( "PIT (x86): [Warning] Attempt to use 'one-shot' mode, but delay exceeds the max allowed delay due to hardware limitations (54.9 ms or 65535 ticks)\n" );
            axk_spinlock_release( &( this->lock ) );
            return AXK_TIMER_ERROR_INVALID_PARAMS;
        }

        axk_x86_outb( PIT_PORT_MODE_COMMAND, 0b00110000 );
        axk_x86_waitio();
        axk_x86_outb( PIT_PORT_CHANNEL_0, (uint8_t)( ticks & 0x00FFUL ) );
        axk_x86_waitio();
        axk_x86_outb( PIT_PORT_CHANNEL_0, (uint8_t)( ( ticks & 0xFF00UL ) >> 8 ) );
        axk_x86_waitio();
        axk_spinlock_release( &( this->lock ) );

        return AXK_TIMER_ERROR_NONE;
    }
    else // AXK_TIMER_MODE_DIVISOR
    {
        // Ensure the divisor is valid
        if( delay > 0xFFFFUL )
        {
            axk_terminal_prints( "PIT (x86): [Warning] Attempt to use 'divisor' mode, but the divisor is out of bounds (Max is 65535)\n" );
            axk_spinlock_release( &( this->lock ) );
            return AXK_TIMER_ERROR_INVALID_PARAMS;
        }

        axk_x86_outb( PIT_PORT_MODE_COMMAND, 0b00110100 );
        axk_x86_waitio();
        axk_x86_outb( PIT_PORT_CHANNEL_0, (uint8_t)( delay & 0x00FFUL ) );
        axk_x86_waitio();
        axk_x86_outb( PIT_PORT_CHANNEL_0, (uint8_t)( ( delay & 0xFF00UL ) >> 8 ) );
        axk_x86_waitio();
        axk_spinlock_release( &( this->lock ) );

        return AXK_TIMER_ERROR_NONE;
    }
}


bool pit_stop( struct axk_timer_driver_t* self )
{
    struct axk_x86_pit_driver_t* this = (struct axk_x86_pit_driver_t*)( self );
    if( this == NULL ) { return false; }

    // Ensure interrupts are disabled while we do this so we arent preempted
    axk_spinlock_acquire( &( this->lock ) );
    axk_x86_outb( PIT_PORT_MODE_COMMAND, 0b00110000 );
    axk_x86_waitio();
    axk_spinlock_release( &( this->lock ) );
    return true;
}


bool pit_is_running( struct axk_timer_driver_t* self )
{
    struct axk_x86_pit_driver_t* this = (struct axk_x86_pit_driver_t*)( self );
    if( this == NULL ) { return false; }

    // TODO: Is there any good way to determine if the timer is still running without the driver handling the interrupt?
    return false;
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

/*==============================================================
    Axon Kernel - x86 LAPIC Timer Driver
    2021, Zachary Berry
    axon/source/arch_x86/lapic_timer_driver.c
==============================================================*/

#include "axon/arch_x86/drivers/lapic_timer_driver.h"
#include "axon/system/interrupts_private.h"
#include "axon/arch_x86/drivers/xapic_driver.h"
#include "axon/arch_x86/util.h"
#include "axon/arch.h"
#include "axon/debug_print.h"
#include "stdlib.h"
#include "big_math.h"

/*
    Constants
*/
#define LAPIC_REGISTER_ID                       0x20
#define LAPIC_REGISTER_LVT_TIMER                0x320
#define LAPIC_REGISTER_TIMER_INIT_COUNT         0x380
#define LAPIC_REGISTER_TIMER_CURRENT_COUNT      0x390
#define LAPIC_REGISTER_TIMER_DIVIDE_CONFIG      0x3E0


bool lapic_timer_init( struct axk_timer_driver_t* self );
bool lapic_timer_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats );
uint32_t lapic_timer_get_id( void );
uint64_t lapic_timer_get_frequency( struct axk_timer_driver_t* self );
uint32_t lapic_timer_start( struct axk_timer_driver_t* self, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, uint32_t processor, uint8_t vector );
bool lapic_timer_stop( struct axk_timer_driver_t* self );
bool lapic_timer_is_running( struct axk_timer_driver_t* self );
uint64_t lapic_timer_get_counter( struct axk_timer_driver_t* self );
uint64_t lapic_timer_get_max_value( struct axk_timer_driver_t* self );

/*
    Create Driver Function
*/
struct axk_timer_driver_t* axk_x86_create_lapic_timer_driver( void )
{
    struct axk_x86_lapic_timer_driver_t* output = AXK_CALLOC_TYPE( struct axk_x86_lapic_timer_driver_t, 1 );

    output->func_table.init             = lapic_timer_init;
    output->func_table.query_features   = lapic_timer_query_features;
    output->func_table.get_id           = lapic_timer_get_id;
    output->func_table.get_frequency    = lapic_timer_get_frequency;
    output->func_table.start            = lapic_timer_start;
    output->func_table.stop             = lapic_timer_stop;
    output->func_table.is_running       = lapic_timer_is_running;
    output->func_table.get_counter      = lapic_timer_get_counter;
    output->func_table.get_max_value    = lapic_timer_get_max_value;

    return (struct axk_timer_driver_t*)( output );
}

static uint32_t calibrate_target;
static uint32_t calibrate_value;
static uint32_t calibrate_ticks;
static struct axk_interrupt_driver_t* calibrate_int_driver;
static bool calibrate_use_x2apic;

static bool calibrate_callback( uint8_t _unused_ )
{
    if( calibrate_ticks == 1 )
    {
        // Read the starting value
        if( calibrate_use_x2apic )
        {
            // TODO
        }
        else
        {
            calibrate_value = axk_x86_xapic_read_lapic( (struct axk_x86_xapic_driver_t*) calibrate_int_driver, LAPIC_REGISTER_TIMER_CURRENT_COUNT );
        }
    }
    else if( calibrate_ticks == calibrate_target )
    {
        // Calculate the delta since the first callback and stop the timer
        if( calibrate_use_x2apic )
        {
            // TODO
        }
        else
        {
            calibrate_value -= axk_x86_xapic_read_lapic( (struct axk_x86_xapic_driver_t*) calibrate_int_driver, LAPIC_REGISTER_TIMER_CURRENT_COUNT );

        }

        axk_timer_stop( axk_timer_get_external() );
    }
    else if( calibrate_ticks > calibrate_target )
    {
        axk_timer_stop( axk_timer_get_external() );
    }

    calibrate_ticks++;
    return false;
}


bool axk_x86_calibrate_lapic_timer( struct axk_timer_driver_t* self )
{
    struct axk_x86_lapic_timer_driver_t* this = (struct axk_x86_lapic_timer_driver_t*)( self );
    if( this == NULL ) { return false; }

    uint32_t crystal_frequency = 0UL;
    uint32_t bus_frequency = 0UL;

    uint32_t eax, ebx, ecx, edx;
    eax = 0x15; 
    if( axk_x86_cpuid_s( &eax, &ebx, &ecx, &edx ) )
    {
        crystal_frequency = ecx;
    }

    eax = 0x15; ebx = 0x00; ecx = 0x00; edx = 0x00;
    if( axk_x86_cpuid_s( &eax, &ebx, &ecx, &edx ) )
    {
        bus_frequency = ( ecx & 0x0000FFFFU ) * 1000000UL;
    }

    // Now were going to benchmark the timer, using an external time source
    // To do this, were going to use one-shot mode, set the highest count possible
    // At the same time, start a periodic external timer, first tick, read the count
    // then the second tick, read the count again, calculate the difference and calculate the rate
    struct axk_timer_driver_t* ptr_timer = axk_timer_get_external();
    if( ptr_timer == NULL ) { return false; }

    calibrate_ticks = 0;
    calibrate_value = 0;

    // This is ugly! 
    calibrate_use_x2apic = ( this->ptr_x2apic != NULL );
    calibrate_int_driver = (struct axk_interrupt_driver_t*)( calibrate_use_x2apic ? (struct axk_interrupt_driver_t*)this->ptr_x2apic : (struct axk_interrupt_driver_t*)this->ptr_xapic );

    // Acquire a local interrupt handler 
    uint8_t int_vector;
    if( !axk_interrupts_acquire_handler( AXK_PROCESS_KERNEL, calibrate_callback, &int_vector ) )
    {
        return false;
    }

    // Start the local APIC timer with a huge starting count
    uint64_t rflags = axk_disable_interrupts();
    uint32_t local_processor;

    if( this->ptr_x2apic != NULL )
    {

    }
    else
    {
        axk_x86_xapic_write_lapic( (struct axk_x86_xapic_driver_t*) this->ptr_xapic, LAPIC_REGISTER_TIMER_DIVIDE_CONFIG, 0b0011 );                  // Divisor is 16
        axk_x86_xapic_write_lapic( (struct axk_x86_xapic_driver_t*) this->ptr_xapic, LAPIC_REGISTER_LVT_TIMER, (uint32_t) AXK_INT_IGNORED );  // Set one shot mode
        axk_x86_xapic_write_lapic( (struct axk_x86_xapic_driver_t*) this->ptr_xapic, LAPIC_REGISTER_TIMER_INIT_COUNT, 0xFFFFFFFF );                 // Initial count is high

        // Read current processor ID
        local_processor = ( ( axk_x86_xapic_read_lapic( (struct axk_x86_xapic_driver_t*) this->ptr_xapic, LAPIC_REGISTER_ID ) & 0xFF000000U ) >> 24 );
    }

    axk_restore_interrupts( rflags );

    if( axk_timer_get_id( ptr_timer ) == AXK_TIMER_ID_PIT ) 
    {
        // 20 ticks, at ~0.05s each
        calibrate_target = 21;
        uint32_t result = axk_timer_start( ptr_timer, AXK_TIMER_MODE_DIVISOR, 59659, false, local_processor, int_vector );
        if( result != AXK_TIMER_ERROR_NONE ) { return false; }
    }
    else
    {
        // 4 ticks, at ~0.25s each
        calibrate_target = 5;
        uint32_t result = axk_timer_start( ptr_timer, AXK_TIMER_MODE_PERIODIC, 250000000, false, local_processor, int_vector );
        if( result != AXK_TIMER_ERROR_NONE ) { return false; }
    }

    // Now, we are going to wait for calibration to complete
    while( calibrate_ticks <= calibrate_target ) { __asm__( "pause" ); }

    // Release the interrupt handler, we dont need it anymore
    axk_interrupts_release_handler( int_vector );

    if( this->ptr_x2apic != NULL )
    {
        // TODO
    }
    else
    {
        axk_x86_xapic_write_lapic( (struct axk_x86_xapic_driver_t*) this->ptr_xapic, LAPIC_REGISTER_TIMER_INIT_COUNT, 0U );
    }

    uint64_t observed = calibrate_value * 16UL;

    // Now, we can compare the rate we observed against any possibly read from CPUID, to get a more accurate result
    if( crystal_frequency != 0UL )
    {
        uint64_t c_diff = observed > crystal_frequency ? observed - crystal_frequency : crystal_frequency - observed;
        if( c_diff < observed / 100UL )
        {
            this->frequency = crystal_frequency;
            axk_terminal_prints( "LAPIC Timer (x86): Acquired rate from the crystal frequency (" );
            axk_terminal_printu64( this->frequency );
            axk_terminal_prints( " Hz)\n" );

            return true;
        }
    }

    // Check against bus frequency
    if( bus_frequency != 0UL )
    {
        uint64_t b_diff = observed > bus_frequency ? observed - bus_frequency : bus_frequency - observed;
        if( b_diff < observed / 100UL )
        {
            this->frequency = bus_frequency;
            axk_terminal_prints( "LAPIC Timer (x86): Acquired rate from system bus frequency (" );
            axk_terminal_printu64( this->frequency );
            axk_terminal_prints( " Hz)\n" );

            return true;
        }
    }

    // Use observed frequency
    this->frequency = observed;
    axk_terminal_prints( "LAPIC Timer (x86): Calibrated rate, observed at " );
    axk_terminal_printu64( this->frequency );
    axk_terminal_prints( " Hz" );

    if( crystal_frequency != 0UL ) { axk_terminal_prints( " (Warning: Not equal to crystal frequency\n" ); }
    else if( bus_frequency != 0UL ) { axk_terminal_prints( " (Warning: Not equal to bus frequency\n" ); }
    else { axk_terminal_printnl(); }

    return true;
}

/*
    Driver Functions
*/
bool lapic_timer_init( struct axk_timer_driver_t* self )
{
    struct axk_x86_lapic_timer_driver_t* this = (struct axk_x86_lapic_timer_driver_t*)( self );
    if( this == NULL || this->b_init ) { return false; }
    this->b_init = true;

    // We need to use the interrupt system driver, because the local timer is driven by it
    struct axk_interrupt_driver_t* ptr_int_driver = axk_interrupts_get();
    switch( axk_interrupts_get_type() )
    {
        case INTERRUPT_DRIVER_X86_XAPIC:
        this->ptr_xapic = (struct axk_x86_xapic_driver_t*)( ptr_int_driver );
        break;

        default:
        break; // TODO: Add x2APIC support
    }

    if( this->ptr_xapic == NULL && this->ptr_x2apic == NULL ) { return false; }

    // Determine if we support deadline mode
    uint32_t eax, ebx, ecx, edx;
    eax = 0x01;
    axk_x86_cpuid( &eax, &ebx, &ecx, &edx );

    this->b_deadline_mode = ( ( ecx & ( 1U << 24 ) ) != 0U );

    // Determine if we are constant
    eax = 0x06; ebx = 0x00; ecx = 0x00; edx = 0x00;
    axk_x86_cpuid( &eax, &ebx, &ecx, &edx );

    this->b_constant = ( ( eax & ( 1U << 2 ) ) != 0U );
    axk_terminal_prints( "LAPIC Timer (x86): Initialized successfully. Constant? " );
    axk_terminal_prints( this->b_constant ? "YES" : "NO" );
    axk_terminal_prints( "  Deadline Mode? " );
    axk_terminal_prints( this->b_deadline_mode ? "YES" : "NO" );
    axk_terminal_printnl();

    return true;
}


bool lapic_timer_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats )
{
    struct axk_x86_lapic_timer_driver_t* this = (struct axk_x86_lapic_timer_driver_t*)( self );
    
    uint32_t features = ( AXK_TIMER_FEATURE_LOCAL | AXK_TIMER_FEATURE_PERIODIC | AXK_TIMER_FEATURE_ONE_SHOT |
        ( this->b_constant ? AXK_TIMER_FEATURE_INVARIANT : AXK_TIMER_FEATURE_NONE ) | 
        ( this->b_deadline_mode ? AXK_TIMER_FEATURE_DEADLINE : AXK_TIMER_FEATURE_NONE ) );
    
    return AXK_CHECK_FLAG( features, (uint32_t) feats );
}


uint32_t lapic_timer_get_id( void )
{
    return AXK_TIMER_ID_LAPIC;
}


uint64_t lapic_timer_get_frequency( struct axk_timer_driver_t* self )
{
    struct axk_x86_lapic_timer_driver_t* this = (struct axk_x86_lapic_timer_driver_t*)( self );
    return this->frequency;
}


uint32_t lapic_timer_start( struct axk_timer_driver_t* self, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, uint32_t processor, uint8_t vector )
{
    struct axk_x86_lapic_timer_driver_t* this = (struct axk_x86_lapic_timer_driver_t*)( self );
    if( this == NULL || delay == 0UL || vector < AXK_INT_MINIMUM) { return AXK_TIMER_ERROR_INVALID_PARAMS; }
    if( mode == AXK_TIMER_MODE_DIVISOR ) { return AXK_TIMER_ERROR_INVALID_MODE; }
    if( mode == AXK_TIMER_MODE_DEADLINE && !this->b_deadline_mode ) { return AXK_TIMER_ERROR_INVALID_MODE; }

    // Calculate delay depending on the mode
    uint64_t delay_value;
    uint8_t divisor = 0b1011;

    if( mode == AXK_TIMER_MODE_ONE_SHOT || mode == AXK_TIMER_MODE_PERIODIC )
    {
        if( b_delay_in_ticks ) { delay_value = delay; }
        else
        {
            // Convert desired delay to ticks
            delay_value = ( this->frequency > ( 0xFFFFFFFFFFFFFFFFUL / delay ) ) ?
                _muldiv64( this->frequency, delay, 1000000000UL ) :
                ( this->frequency * delay ) / 1000000000UL;
        }

        if( delay_value > 0x7FFFFFFF80UL )
        {
            return AXK_TIMER_ERROR_INVALID_PARAMS;
        }
        else if( delay_value > 0x3FFFFFFFC0UL )
        {
            // Use divisor of 128
            divisor = 0b1010;
            delay_value /= 128;
        }
        else if( delay_value > 0x1FFFFFFFE0UL )
        {
            // Use divisor of 64
            divisor = 0b1001;
            delay_value /= 64;
        }
        else if( delay_value > 0xFFFFFFFF0UL )
        {
            // Use divisor of 32
            divisor = 0b1000;
            delay_value /= 32;
        }
        else if( delay_value > 0x7FFFFFFF8UL )
        {
            // Use divisor of 16
            divisor = 0b0011;
            delay_value /= 16;
        }
        else if( delay_value > 0x3FFFFFFFCUL )
        {
            // Use divisor of 8
            divisor = 0b0010;
            delay_value /= 8;
        }
        else if( delay_value > 0x1FFFFFFFEUL )
        {
            // Use divisor of 4
            divisor = 0b0001;
            delay_value /= 4;
        }
        else if( delay_value > 0xFFFFFFFFUL )
        {
            // Use divisor of 2
            divisor = 0b0000;
            delay_value /= 2;
        }
        else
        {
            // Use divisor of 1
            divisor = 0b1011;
        }
    }
    else if( mode == AXK_TIMER_MODE_DEADLINE )
    {
        uint64_t cur_time = axk_x86_read_timestamp();

        if( b_delay_in_ticks ) { delay_value = cur_time + delay; }
        else
        {
            uint64_t tsc_freq = axk_timer_get_frequency( axk_timer_get_counter() );
            delay_value = cur_time + _muldiv64( tsc_freq, 1000000000UL, delay );
        }
    }

    // Disable interrupts
    uint64_t rflags = axk_disable_interrupts();

    // Stop existing timer (caller needs to ensure timer isnt already running)
    // Then, set the proper mode
    uint32_t lvt_value = (uint32_t) vector;
    switch( mode )
    {
        case AXK_TIMER_MODE_ONE_SHOT:
        break;
        case AXK_TIMER_MODE_DEADLINE:
        lvt_value |= ( 0b10U << 17 );
        break;
        case AXK_TIMER_MODE_PERIODIC:
        lvt_value |= ( 0b01U << 17 );
        break;

    }

    if( this->ptr_x2apic != NULL ) 
    { 
        /* TODO */ 
    }
    else 
    { 
        axk_x86_xapic_write_lapic( this->ptr_xapic, LAPIC_REGISTER_TIMER_DIVIDE_CONFIG, divisor );
        axk_x86_xapic_write_lapic( this->ptr_xapic, LAPIC_REGISTER_LVT_TIMER, lvt_value ); 
    }

    // Start the timer
    if( mode == AXK_TIMER_MODE_ONE_SHOT || mode == AXK_TIMER_MODE_PERIODIC )
    {
        if( this->ptr_x2apic != NULL )
        {
            // TODO
        }
        else
        {
            axk_x86_xapic_write_lapic( this->ptr_xapic, LAPIC_REGISTER_TIMER_INIT_COUNT, delay_value );
        }
    }
    else
    {
        // Deadline mode
        axk_x86_write_msr( 0x6E0, delay_value );
    }

    axk_restore_interrupts( rflags );
    return AXK_TIMER_ERROR_NONE;
}


bool lapic_timer_stop( struct axk_timer_driver_t* self )
{
    struct axk_x86_lapic_timer_driver_t* this = (struct axk_x86_lapic_timer_driver_t*)( self );

    // Determine the current mode
    uint64_t rflags = axk_disable_interrupts();

    if( this->ptr_x2apic != NULL )
    {
        // TODO
    }
    else
    {
        uint32_t val = ( ( axk_x86_xapic_read_lapic( this->ptr_xapic, LAPIC_REGISTER_LVT_TIMER ) & ( 0b11 << 17 ) ) >> 17 );
        switch( val )
        {
            case 0b00:
            axk_x86_xapic_write_lapic( this->ptr_xapic, LAPIC_REGISTER_TIMER_INIT_COUNT, 0U );
            break;

            case 0b10:
            axk_x86_write_msr( 0x6E0, 0UL );
            break;

            case 0b01:
            axk_x86_xapic_write_lapic( this->ptr_xapic, LAPIC_REGISTER_TIMER_INIT_COUNT, 0U );
            break;
        }
    }

    axk_restore_interrupts( rflags );
    return true;
}


bool lapic_timer_is_running( struct axk_timer_driver_t* self )
{
    struct axk_x86_lapic_timer_driver_t* this = (struct axk_x86_lapic_timer_driver_t*)( self );

    // Determine current mode
    uint64_t rflags = axk_disable_interrupts();
    bool result;

    if( this->ptr_x2apic != NULL )
    {
        // TODO
    }
    else
    {
        uint32_t mode = ( ( axk_x86_xapic_read_lapic( this->ptr_xapic, LAPIC_REGISTER_LVT_TIMER ) & ( 0b11 << 17 ) ) >> 17 );
        switch( mode )
        {
            case 0b00:
            result = axk_x86_xapic_read_lapic( this->ptr_xapic, LAPIC_REGISTER_TIMER_CURRENT_COUNT ) > 0U;

            case 0b01:
            result = axk_x86_xapic_read_lapic( this->ptr_xapic, LAPIC_REGISTER_TIMER_CURRENT_COUNT ) > 0U;

            case 0b10:
            result = axk_x86_read_msr( 0x6E0 ) > 0UL;
        }
    }

    axk_restore_interrupts( rflags );
    return result;
}


uint64_t lapic_timer_get_counter( struct axk_timer_driver_t* self )
{
    struct axk_x86_lapic_timer_driver_t* this = (struct axk_x86_lapic_timer_driver_t*)( self );

    uint64_t rflags = axk_disable_interrupts();
    uint64_t output;

    if( this->ptr_x2apic != NULL )
    {
        // TODO
    }
    else
    {
        output = axk_x86_xapic_read_lapic( this->ptr_xapic, LAPIC_REGISTER_TIMER_CURRENT_COUNT );
    }

    axk_restore_interrupts( rflags );
    return output;
}


uint64_t lapic_timer_get_max_value( struct axk_timer_driver_t* self )
{
    return 0xFFFFFFFFUL;
}

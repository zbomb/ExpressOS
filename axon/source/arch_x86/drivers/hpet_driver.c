/*==============================================================
    Axon Kernel - x86 HPET Driver
    2021, Zachary Berry
    axon/source/arch_x86/hpet_driver.c
==============================================================*/

#include "axon/arch_x86/drivers/hpet_driver.h"
#include "axon/arch_x86/acpi_info.h"
#include "axon/memory/kmap.h"
#include "axon/system/interrupts.h"
#include "axon/debug_print.h"
#include "axon/system/processor_info.h"
#include "axon/arch.h"
#include "stdlib.h"
#include "big_math.h"

/*
    Constants
*/
#define HPET_64BIT_COUNTER_MAX      0xFFFFFFFFFFFFFFFFUL
#define HPET_32BIT_COUNTER_MAX      0xFFFFFFFFU

#define HPET_REGISTER_CAPABILITIES  0x00UL
#define HPET_REGISTER_CONFIG        0x010UL
#define HPET_REGISTER_INT_STATUS    0x020UL
#define HPET_REGISTER_COUNTER       0x0F0UL

#define HPET_FEMTO_PER_SECOND       1000000000000000UL
#define HPET_FEMTO_PER_NANOSECOND   1000000UL

#define HPET_TIMER_FLAG_IS_PERIODIC      ( 1UL << 4 )
#define HPET_TIMER_FLAG_IS_64BIT         ( 1UL << 5 )
#define HPET_TIMER_FLAG_ALLOWS_FSB       ( 1UL << 15 )


/*
    Create Driver Function
*/
bool hpet_init( struct axk_timer_driver_t* self );
bool hpet_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats );
uint32_t hpet_get_id( void );
uint64_t hpet_get_frequency( struct axk_timer_driver_t* self );
uint32_t hpet_start( struct axk_timer_driver_t* self, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, bool( *callback )( void ) );
bool hpet_stop( struct axk_timer_driver_t* self );
bool hpet_is_running( struct axk_timer_driver_t* self );
uint64_t hpet_get_counter( struct axk_timer_driver_t* self );
uint64_t hpet_get_max_value( struct axk_timer_driver_t* self );
bool hpet_invoke( struct axk_timer_driver_t* self );


struct axk_timer_driver_t* axk_x86_create_hpet_driver( struct axk_x86_hpet_info_t* ptr_info )
{
    if( ptr_info == NULL ) { return NULL; }

    struct axk_x86_hpet_driver_t* output = AXK_CALLOC_TYPE( struct axk_x86_hpet_driver_t, 1 );

    output->func_table.init             = hpet_init;
    output->func_table.query_features   = hpet_query_features;
    output->func_table.get_id           = hpet_get_id;
    output->func_table.get_frequency    = hpet_get_frequency;
    output->func_table.start            = hpet_start;
    output->func_table.stop             = hpet_stop;
    output->func_table.is_running       = hpet_is_running;
    output->func_table.get_counter      = hpet_get_counter;
    output->func_table.get_max_value    = hpet_get_max_value;
    output->func_table.invoke           = hpet_invoke;

    output->info = ptr_info;

    return (struct axk_timer_driver_t*)( output );
}

/*
    Structures
*/
// Maybe, we cant read the whole timer register as a uint64_t?
// Maybe, we should write a proper structure, and read the fields we need
// Possibly, reading from the reserved bits will throw up some errors

/*
    Helper Functions
*/
uint64_t hpet_read_general_register( struct axk_x86_hpet_driver_t* this, uint32_t offset )
{
    return *( (uint64_t volatile*)( this->base_address + (uint64_t)offset ) );
}

void hpet_write_general_register( struct axk_x86_hpet_driver_t* this, uint32_t offset, uint64_t in )
{
    *( (uint64_t volatile*)( this->base_address + (uint64_t)offset ) ) = in;
}

uint64_t hpet_read_timer_config( struct axk_x86_hpet_driver_t* this, uint8_t timer )
{
    return *( (uint64_t volatile*)( this->base_address + 0x100UL + ( 0x20UL * (uint64_t) timer ) ) );
}

void hpet_write_timer_config( struct axk_x86_hpet_driver_t* this, uint8_t timer, uint64_t value )
{
    *( (uint64_t volatile*)( this->base_address + 0x100UL + ( 0x20UL * (uint64_t) timer ) ) ) = value;
}

uint64_t hpet_read_comparator( struct axk_x86_hpet_driver_t* this, uint8_t timer )
{
    return *( (uint64_t volatile*)( this->base_address + 0x108UL + ( 0x20UL * (uint64_t) timer ) ) );
}

void hpet_write_comparator( struct axk_x86_hpet_driver_t* this, uint8_t timer, uint64_t value )
{
    *( (uint64_t volatile*)( this->base_address + 0x108UL + ( 0x20UL * (uint64_t) timer ) ) ) = value;
}

void hpet_write_comparator_low( struct axk_x86_hpet_driver_t* this, uint8_t timer, uint32_t low )
{
    *( (uint32_t volatile*)( this->base_address + 0x108UL + ( 0x20UL * (uint64_t) timer ) ) ) = low;
}

void hpet_write_comparator_high( struct axk_x86_hpet_driver_t* this, uint8_t timer, uint32_t high )
{
    *( (uint32_t volatile*)( this->base_address + 0x108UL + ( 0x20UL * (uint64_t) timer ) ) + 0x4UL ) = high;
}

uint64_t hpet_read_timer_routing( struct axk_x86_hpet_driver_t* this, uint8_t timer )
{
    return *( (uint64_t volatile*)( this->base_address + 0x110UL + ( 0x20UL * (uint64_t) timer ) ) );
}

void hpet_write_timer_routing( struct axk_x86_hpet_driver_t* this, uint8_t timer, uint64_t value )
{
    *( (uint64_t volatile*)( this->base_address + 0x110UL + ( 0x20UL * (uint64_t) timer ) ) ) = value;
}


/*
    Driver Functions
*/
bool hpet_callback( uint8_t vec )
{
    return false;
}


bool hpet_init( struct axk_timer_driver_t* self )
{
    struct axk_x86_hpet_driver_t* this = (struct axk_x86_hpet_driver_t*)( self );
    if( this == NULL || this->info == NULL ) { return false; }

    this->min_tick      = this->info->min_tick;
    this->base_address  = this->info->address;
    this->callback      = NULL;

    uint64_t begin_addr     = this->base_address;
    uint64_t end_addr       = this->base_address + 0x517UL;

    // The registers used to access the HPET is memory mapped, so we need to ensure we have these memory addresses mapped somewhere usable
    bool b_split_page = ( ( begin_addr / 0x1000UL ) != ( end_addr / 0x1000UL ) );
    uint64_t virt_addr = axk_acquire_shared_address( b_split_page ? 1UL : 2UL );

    if( !axk_kmap( begin_addr / 0x1000UL, virt_addr, AXK_FLAG_PAGEMAP_DISABLE_CACHE ) )
    {
        return false;
    }

    if( b_split_page )
    {
        // Incase the registers are split across two pages, we need to map both of them
        if( !axk_kmap( end_addr / 0x1000UL, virt_addr + AXK_PAGE_SIZE, AXK_FLAG_PAGEMAP_DISABLE_CACHE ) )
        {
            return false;
        }
    }

    this->base_address = ( begin_addr % 0x1000UL ) + virt_addr;

    // Read capabilities register
    uint64_t capabilities   = hpet_read_general_register( this, HPET_REGISTER_CAPABILITIES );
    uint8_t timer_count     = (uint8_t)( ( capabilities & 0b1111100000000 ) >> 8 ) + 1;
    this->period            = (uint64_t)( ( capabilities & 0xFFFFFFFF00000000UL ) >> 32 );
    this->b_long_counter    = ( ( capabilities & ( 1UL << 13 ) ) != 0UL );


    // Ensure the timer is turned off, legacy mode off
    uint64_t config = hpet_read_general_register( this, HPET_REGISTER_CONFIG );

    if( ( ( config & 0b10 ) != 0UL ) )
    {
        axk_terminal_prints( "HPET (x86): Disabling legacy routing mode\n" );
    }

    hpet_write_general_register( this, HPET_REGISTER_CONFIG, 0UL );

    // Now, we can actually have multiple 'comparators' at once, where once the counter hits a value, an interrupt is generated
    // But, there is a single main counter. The problem really is, this counter MIGHT be 32-bits
    // So supporting multiple comparators at once might be a little more complex
    // For now, were just going to find the BEST comparator and use it
    this->timer_index           = 0U;
    uint32_t ioapic_routings    = 0U;
    bool b_found_periodic       = false;
    bool b_found_long           = false;
    bool b_found_fsb            = false;
    bool b_found_timer          = false;

    for( uint8_t i = 0; i < timer_count; i++ )
    {
        uint64_t timer_cfg = hpet_read_timer_config( this, i );
        if( !b_found_timer ) 
        { 
            b_found_timer       = true; 
            this->timer_index   = i; 
            ioapic_routings     = (uint32_t)( ( timer_cfg & 0xFFFFFFFF00000000UL ) >> 32 );
        }

        // Check if this timer is a periodic timer
        if( AXK_CHECK_FLAG( timer_cfg, HPET_TIMER_FLAG_IS_PERIODIC ) )
        {
            if( !b_found_periodic )
            {
                b_found_periodic    = true;
                this->timer_index   = i;
                ioapic_routings     = (uint32_t)( ( timer_cfg & 0xFFFFFFFF00000000UL ) >> 32 );
            }

            // Check if this timer supports a 64-bit value
            if( AXK_CHECK_FLAG( timer_cfg, HPET_TIMER_FLAG_IS_64BIT ) )
            {
                if( !b_found_long )
                {
                    b_found_long        = true;
                    this->timer_index   = i;
                    ioapic_routings     = (uint32_t)( ( timer_cfg & 0xFFFFFFFF00000000UL ) >> 32 );
                }

                // Check if this timer supports FSB
                if( AXK_CHECK_FLAG( timer_cfg, HPET_TIMER_FLAG_ALLOWS_FSB ) )
                {
                    if( !b_found_fsb )
                    {
                        b_found_fsb         = true;
                        this->timer_index   = i;
                        ioapic_routings     = (uint32_t)( ( timer_cfg & 0xFFFFFFFF00000000UL ) >> 32 );
                        break; // Not gonna find anything better!
                    }
                }
            }
        }
    }

    if( !b_found_timer || !b_found_periodic )
    {
        axk_terminal_prints( "HPET (x86): There are no timers that support periodic mode! \n" );
        return false;
    }

    this->b_timer_long      = b_found_long;
    this->b_timer_fsb       = b_found_fsb;

    // Acquire global interrupt routing vector
    struct axk_external_interrupt_routing_t routing;
    routing.local_interrupt     = AXK_INT_EXTERNAL_TIMER;
    routing.b_low_priority      = false;
    routing.b_active_low        = false;
    routing.b_level_triggered   = false;
    routing.b_masked            = false;
    routing.target_processor    = axk_processor_get_id();

    // Build a list of allowed external routings so we can acquire one we can actually use
    uint32_t count = 0;
    for( uint8_t i = 0; i < 32; i++ )
    {
        if( ( ioapic_routings & ( 1U << i ) ) != 0U )
        {
            count++;
        }
    }

    uint32_t* ext_list = (uint32_t*) calloc( count, sizeof( uint32_t ) );
    count = 0;

    for( uint8_t i = 0; i < 32; i++ )
    {
        if( ( ioapic_routings & ( 1U << i ) ) != 0U )
        {
            ext_list[ count++ ] = (uint32_t) i;
        }
    }

    // Attempt to acquire external interrupt routing
    if( !axk_interrupts_acquire_external_clamped( AXK_PROCESS_KERNEL, &routing, ext_list, count ) )
    {
        return false;
    }

    // And write the timer config, for some reason with BOCHS, setting the timer to 32-bit, and back to 64-bit fixes issues with writing to its comparator
    hpet_write_timer_config( this, this->timer_index, ( (uint64_t)( routing.global_interrupt ) << 9 ) | ( 1UL << 8 ) );
    hpet_write_timer_config( this, this->timer_index, ( (uint64_t)( routing.global_interrupt ) << 9 ) );

    this->global_interrupt = routing.global_interrupt;
    this->b_running = false;
    axk_spinlock_init( &( this->lock ) );

    axk_terminal_prints( "HPET (x86): Initialized. 64-bit? " );
    axk_terminal_prints( this->b_long_counter ? "YES" : "NO" );
    axk_terminal_prints( "  FSB Routing? " );
    axk_terminal_prints( this->b_timer_fsb ? "YES" : "NO" );
    axk_terminal_prints( "  Index: " );
    axk_terminal_printu32( this->timer_index );
    axk_terminal_prints( "  Global Int: " );
    axk_terminal_printu32( routing.global_interrupt );
    axk_terminal_prints( "  Comparator 64-bit? " );
    axk_terminal_prints( this->b_timer_long ? "YES" : "NO" );
    axk_terminal_printnl();

    return true;
}


bool hpet_query_features( struct axk_timer_driver_t* self, enum axk_timer_features_t feats )
{
    uint32_t features = AXK_TIMER_FEATURE_ONE_SHOT | AXK_TIMER_FEATURE_PERIODIC | AXK_TIMER_FEATURE_EXTERNAL | AXK_TIMER_FEATURE_INVARIANT;
    return AXK_CHECK_FLAG( features, (uint32_t) feats );
}


uint32_t hpet_get_id( void )
{
    return AXK_TIMER_ID_HPET;
}


uint64_t hpet_get_frequency( struct axk_timer_driver_t* self )
{
    struct axk_x86_hpet_driver_t* this = (struct axk_x86_hpet_driver_t*)( self );
    return ( HPET_FEMTO_PER_SECOND / this->period );
}


uint32_t hpet_start( struct axk_timer_driver_t* self, enum axk_timer_mode_t mode, uint64_t delay, bool b_delay_in_ticks, bool( *callback )( void ) )
{
    struct axk_x86_hpet_driver_t* this = (struct axk_x86_hpet_driver_t*)( self );

    // Validate parameters
    if( this == NULL || callback == NULL || delay == 0UL ) { return AXK_TIMER_ERROR_INVALID_PARAMS; }
    if( mode == AXK_TIMER_MODE_DEADLINE || mode == AXK_TIMER_MODE_DIVISOR ) { return AXK_TIMER_ERROR_INVALID_MODE; }

    // Convert delay to ticks
    uint64_t ticks;
    if( b_delay_in_ticks )
    {
        ticks = delay;
    }
    else
    {
        ticks = _muldiv64( delay, HPET_FEMTO_PER_NANOSECOND, this->period );
    }

    // Check if the delay is too small?
    // TODO: Is this correct?
    if( ticks < (uint64_t) this->min_tick )
    {
        callback();
        return AXK_TIMER_ERROR_NONE;
    }
    else if( ticks > 0xFFFFFFFFUL && !this->b_timer_long )
    {
        return AXK_TIMER_ERROR_INVALID_PARAMS;
    }

    // Acquire lock and check if were running
    axk_spinlock_acquire( &( this->lock ) );
    if( this->b_running )
    {
        axk_spinlock_release( &( this->lock ) );
        return AXK_TIMER_ERROR_ALREADY_RUNNING;
    }

    this->b_running = true;
    this->callback = callback;

    // Disable the global counter, and disable the timer
    hpet_write_general_register( this, HPET_REGISTER_CONFIG, 0UL );
    hpet_write_general_register( this, HPET_REGISTER_COUNTER, 0UL );

    if( mode == AXK_TIMER_MODE_ONE_SHOT )
    {
        // One-Shot Timer Mode
        hpet_write_timer_config( this, this->timer_index, ( (uint64_t)( this->global_interrupt ) << 9 ) | ( 1UL << 2 ) );
        hpet_write_comparator( this, this->timer_index, ticks );
    }
    else
    {
        // Periodic Timer Mode
        this->b_running_periodic = true;
        uint64_t timer_config = ( (uint64_t)( this->global_interrupt ) << 9 ) | ( 1UL << 2 ) | ( 1UL << 3 ) | ( 1UL << 6 );
        hpet_write_timer_config( this, this->timer_index, timer_config );
        hpet_write_comparator_low( this, this->timer_index, (uint32_t)( ticks & 0x00000000FFFFFFFFUL ) );
        hpet_write_timer_config( this, this->timer_index, timer_config );
        hpet_write_comparator_high( this, this->timer_index, (uint32_t)( ( ticks & 0xFFFFFFFF00000000UL ) >> 32 ) );
    }

    hpet_write_general_register( this, HPET_REGISTER_CONFIG, 1UL );
    axk_spinlock_release( &( this->lock ) );
    return AXK_TIMER_ERROR_NONE;
}


bool hpet_stop( struct axk_timer_driver_t* self )
{
    struct axk_x86_hpet_driver_t* this = (struct axk_x86_hpet_driver_t*)( self );
    if( this == NULL ) { return false; }

    // Acquire lock, stop the timer and update state
    axk_spinlock_acquire( &( this->lock ) );

    // Disable interrupts on this timer
    hpet_write_timer_config( this, this->timer_index, 
        hpet_read_timer_config( this, this->timer_index ) & 0xFFFFFFFFFFFFFFFBUL
    );

    this->b_running = false;
    this->b_running_periodic = false;
    this->callback = NULL;

    axk_spinlock_release( &( this->lock ) );
    return true;
}


bool hpet_is_running( struct axk_timer_driver_t* self )
{
    struct axk_x86_hpet_driver_t* this = (struct axk_x86_hpet_driver_t*)( self );
    if( this == NULL ) { return false; }
    return this->b_running;
}


uint64_t hpet_get_counter( struct axk_timer_driver_t* self )
{
    struct axk_x86_hpet_driver_t* this = (struct axk_x86_hpet_driver_t*)( self );
    if( this == NULL ) { return false; }

    // Acquire lock, read mian counter
    axk_spinlock_acquire( &( this->lock ) );

    uint64_t counter = hpet_read_general_register( this, HPET_REGISTER_COUNTER );

    axk_spinlock_release( &( this->lock ) );

    return counter;
}


uint64_t hpet_get_max_value( struct axk_timer_driver_t* self )
{
    struct axk_x86_hpet_driver_t* this = (struct axk_x86_hpet_driver_t*)( self );
    return this->b_long_counter ? 0xFFFFFFFFFFFFFFFFUL : 0xFFFFFFFFUL;
} 


bool hpet_invoke( struct axk_timer_driver_t* self )
{
    struct axk_x86_hpet_driver_t* this = (struct axk_x86_hpet_driver_t*)( self );

    axk_spinlock_acquire( &( this->lock ) );
    void* callback = this != NULL ? this->callback : NULL;
    if( !this->b_running_periodic ) { this->b_running = false; this->callback = NULL; }
    axk_spinlock_release( &( this->lock ) );

    if( callback != NULL )
    {
        return ( (bool(*)(void))( callback ) )();
    }

    return false; // Didnt send EOI
}
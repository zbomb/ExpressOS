/*==============================================================
    Axon Kernel - Time Structures
    2021, Zachary Berry
    axon/source/system/time.c
==============================================================*/

#include "axon/system/time.h"
#include "axon/library/spinlock.h"
#include "axon/system/timers.h"
#include "axon/system/time_private.h"
#include "axon/system/interrupts.h"
#include "axon/system/sysinfo_private.h"
#include "axon/panic.h"
#include "big_math.h"
#include "string.h"

/*
    axk_time_sync_point_t
    * Private Structure
    * Used to store info about the last time the external timer 'ticked'
*/
struct axk_time_sync_point_t
{
    uint64_t counter_value;
    uint64_t counter_rate;
    uint64_t since_boot;
};

/*
    State
*/
static struct axk_spinlock_t g_sync_lock;
static struct axk_time_sync_point_t g_sync_point;
static uint64_t g_sync_history[ 6 ];
static uint64_t g_utc_offset;
static struct axk_atomic_uint64_t g_last_time;
static uint64_t g_timer_period;
static uint64_t g_ext_tick_counter;

/*
    Function Implementations
*/
void axk_time_init( uint64_t ext_tick_period )
{
    // Setup the initial state
    memset( (void*) g_sync_history, 0, sizeof( uint64_t ) * 6UL );
    axk_spinlock_init( &g_sync_lock );
    axk_atomic_store_uint64( &g_last_time, 0UL, MEMORY_ORDER_SEQ_CST );

    g_utc_offset        = 0UL;
    g_timer_period      = ext_tick_period;
    g_ext_tick_counter  = 0UL;

    g_sync_point.counter_value  = 0UL;
    g_sync_point.counter_rate   = 0UL;
    g_sync_point.since_boot     = 0UL;

    // Initialize the persistent hardware clock (arch-specific)
    axk_time_init_persistent_clock();
}


void axk_time_ext_tick( void )
{
    // This is directly called by the external timer
    // Ignore the first couple ticks, since they might not be perfectly timed
    if( g_ext_tick_counter > 2UL )
    {
        struct axk_timer_driver_t* ptr_counter = axk_timer_get_counter();
        uint64_t new_counter = axk_timer_get_counter_value( ptr_counter );

        // Update the system clock
        axk_spinlock_acquire( &g_sync_lock );

        // Calculate average frequency over the last 6 ticks (0.25s)
        g_sync_history[ 5 ] = g_sync_history[ 4 ];
        g_sync_history[ 4 ] = g_sync_history[ 3 ];
        g_sync_history[ 3 ] = g_sync_history[ 2 ];
        g_sync_history[ 2 ] = g_sync_history[ 1 ];
        g_sync_history[ 1 ] = g_sync_history[ 0 ];
        g_sync_history[ 0 ] = new_counter;
        
        // Ensure the history table is filled before we start re-calculating frequency
        if( g_ext_tick_counter > 8UL )
        {
            uint64_t counter_diff       = g_sync_history[ 0 ] - g_sync_history[ 5 ];
            g_sync_point.counter_rate   = ( counter_diff > ( 0xFFFFFFFFFFFFFFFFUL / 1000000000UL ) ) ? 
                _muldiv64( counter_diff, 1000000000UL, 5UL * g_timer_period ) :
                ( counter_diff * 1000000000UL ) / ( 5UL * g_timer_period );
        }

        g_sync_point.counter_value  = g_sync_history[ 0 ];
        g_sync_point.since_boot     += g_timer_period;

        axk_spinlock_release( &g_sync_lock );
        axk_counter_increment( AXK_COUNTER_EXT_CLOCK_TICKS, 1UL );
    }
    else if( g_ext_tick_counter == 2UL )
    {
        // Syncronize with persistent hardware clock to get the 'UTC Offset'
        struct axk_timer_driver_t* ptr_counter  = axk_timer_get_counter();
        uint64_t counter_tick                   = axk_timer_get_counter_value( ptr_counter );

        uint64_t counter_read;
        struct axk_date_t utc_date;
        struct axk_time_t utc_time;

        if( !axk_time_read_persistent_clock( &utc_date, &counter_read ) || !axk_date_to_time( &utc_date, &utc_time ) )
        {
            // TODO: We dont actually need to panic here...
            axk_panic( "Time: failed to read the persistent hardware clock" );
        }

        g_sync_point.counter_value  = counter_tick;
        g_sync_point.counter_rate   = axk_timer_get_frequency( ptr_counter );
        g_sync_point.since_boot     = 0UL;

        // Convert the difference between reading the clock, and the start of the tick to nanoseconds to get a more accurate UTC offset
        g_utc_offset = utc_time.raw - ( ( ( counter_read - counter_tick ) * 1000000000UL ) / g_sync_point.counter_rate );
    }

    g_ext_tick_counter++;
    axk_interrupts_signal_eoi();
}


void axk_time_wait_for_sync( void )
{
    // Wait for the external tick counter to hit at least 3, so we know UTC sync is complete
    while( g_ext_tick_counter < 3UL ) { __asm__( "pause" ); }
}


uint64_t axk_time_get_since_boot( void )
{
    // Get the current counter driver
    struct axk_timer_driver_t* ptr_counter = axk_timer_get_counter();
    
    // Read last sync point
    struct axk_time_sync_point_t last_sync;
    axk_spinlock_acquire( &g_sync_lock );
    memcpy( (void*)( &last_sync ), (void*)( &g_sync_point ), sizeof( struct axk_time_sync_point_t ) );
    axk_spinlock_release( &g_sync_lock );

    // Calculate the counter change since the last sync point, taking into account wrap around
    uint64_t new_value = ptr_counter->get_counter( ptr_counter );
    uint64_t delta_value = ( new_value < last_sync.counter_value ) ? 
        ( ptr_counter->get_max_value( ptr_counter ) - last_sync.counter_value ) + new_value :
        new_value - last_sync.counter_value;

    // Convert this to nanoseconds...
    uint64_t counter_freq = last_sync.counter_rate == 0UL ? ptr_counter->get_frequency( ptr_counter ) : last_sync.counter_rate;
    uint64_t out_nano = ( delta_value > ( 0xFFFFFFFFFFFFFFFFUL / 1000000000UL ) ) ? 
        _muldiv64( delta_value, 1000000000UL, counter_freq ) :
        ( delta_value * 1000000000UL ) / counter_freq;

    // Calculate the final value
    out_nano += last_sync.since_boot;

    // Now, were going to ensure this function returns a monotonic value even when a 'sync' happens 
    // To do this, we track the last value output by this function and ensure we are at least one greater than it
    uint64_t min_output = axk_atomic_fetch_add_uint64( &g_last_time, 1UL, MEMORY_ORDER_SEQ_CST ) + 1UL;
    if( min_output >= out_nano ) { return min_output; }

    while( !axk_atomic_cmpexchg_uint64( &g_last_time, &min_output, out_nano, true, MEMORY_ORDER_SEQ_CST, MEMORY_ORDER_SEQ_CST ) )
    {
        min_output = axk_atomic_fetch_add_uint64( &g_last_time, 1UL, MEMORY_ORDER_SEQ_CST ) + 1UL;
        if( min_output >= out_nano ) { return min_output; }
    }

    return out_nano;
}


void axk_time_get( struct axk_time_t* out_time )
{
    if( out_time == NULL ) { return; }
    out_time->raw = axk_time_get_since_boot() + g_utc_offset;
}


void axk_date_get( struct axk_date_t* out_date )
{
    if( out_date == NULL ) { return; }

    struct axk_time_t t;
    axk_time_get( &t );
    axk_time_to_date( &t, out_date );
}


void axk_time_convert_duration( struct axk_duration_t* in_value, enum axk_time_unit_t out_unit, struct axk_duration_t* out_value )
{
    out_value->value    = axk_time_convert( in_value->value, in_value->unit, out_unit );
    out_value->unit     = out_unit;
}

uint64_t axk_time_convert( uint64_t in_value, enum axk_time_unit_t in_unit, enum axk_time_unit_t out_unit )
{
    uint64_t source_mult;
    switch( in_unit )
    {
        case AXK_TIME_UNIT_NANOSECOND:
        source_mult = 1UL;
        break;
        case AXK_TIME_UNIT_MICROSECOND:
        source_mult = 1000UL;
        break;
        case AXK_TIME_UNIT_MILLISECOND:
        source_mult = 1000000UL;
        break;
        case AXK_TIME_UNIT_SECOND:
        source_mult = 1000000000UL;
        break;
        case AXK_TIME_UNIT_MINUTE:
        source_mult = 60000000000UL;
        break;
        case AXK_TIME_UNIT_HOUR:
        source_mult = 3600000000000UL;
        break;
    }

    uint64_t dest_div;
    switch( out_unit )
    {
        case AXK_TIME_UNIT_NANOSECOND:
        dest_div = 1UL;
        break;
        case AXK_TIME_UNIT_MICROSECOND:
        dest_div = 1000UL;
        break;
        case AXK_TIME_UNIT_MILLISECOND:
        dest_div = 1000000UL;
        break;
        case AXK_TIME_UNIT_SECOND:
        dest_div = 1000000000UL;
        break;
        case AXK_TIME_UNIT_MINUTE:
        dest_div = 60000000000UL;
        break;
        case AXK_TIME_UNIT_HOUR:
        dest_div = 3600000000000UL;
        break;
    }

    return( source_mult == dest_div ? in_value : _muldiv64( in_value, source_mult, dest_div ) );
}

static const uint8_t _non_leap_year[]   = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const uint8_t _leap_year[]       = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


void axk_time_to_date( struct axk_time_t* in_time, struct axk_date_t* out_date )
{
    // First, convert the source date to nanoseconds
    uint64_t source = in_time->raw;

    // Going the other way is a bit harder, what we can do is, loop through and calculate how many seconds per year, and subtract
    // that number from the source, until theres not enough time left for another full year
    out_date->year = 2000;

    for( uint32_t i = 2000; i < 4000; i++ )
    {
        // We need to calculate the number of seconds in this year... to do this, we need to determine if there...
        // A) There is a leap day.. and B) If there is a leap second
        bool b_leap_year = ( ( ( i - 2000 ) % 4 ) == 0 );
        bool b_leap_second = false;

        if( i == 2005 || i == 2008 || i == 2015 || i == 2016 ) { b_leap_second = true; }

        uint64_t nano_this_year = ( b_leap_year ? 366 : 365 ) * 86400UL * 1000000000UL;
        if( b_leap_second ) { nano_this_year += 1000000000UL; }

        if( source >= nano_this_year ) 
        { 
            source -= nano_this_year;
            out_date->year++; 
        }
        else
        {
            break;
        }
    }

    // Now, we need to calculate the month, its similar to the year, we calculate the number of nanoseconds in each month
    // and subtract from nano, until theres less nanosecnods left than the next month
    out_date->month = 1;
    bool b_leap = ( ( ( out_date->year - 2000 ) % 4 ) == 0 );
    const uint8_t* month_list = b_leap ? _leap_year : _non_leap_year;

    for( uint8_t i = 0; i < 12; i++ )
    {
        uint8_t days_this_month   = month_list[ i ];
        uint64_t nano_this_month  = days_this_month * 86400UL * 1000000000UL;

        // There were a couple months where a leap second was added in the middle of a year
        if( out_date->year == 2012 && out_date->month == 6 ) { nano_this_month += 1000000000UL; }
        if( out_date->year == 2015 && out_date->month == 6 ) { nano_this_month += 1000000000UL; }

        if( source >= nano_this_month ) 
        {
            source -= nano_this_month;
            out_date->month++;
        }
        else
        {
            break;
        }
    }

    // Now, calculate what day it is in the month
    out_date->day   = ( source / 86400000000000UL );
    source          -= ( out_date->day * 86400000000000UL );
    out_date->day++;

    // Calculate what hour it is
    out_date->hour  = ( source / 3600000000000UL );
    source          -= ( out_date->hour * 3600000000000UL );

    // Calculate the minute
    out_date->minute    = ( source / 60000000000UL );
    source              -= ( out_date->minute * 60000000000UL );

    // Calculate the second
    out_date->second        = ( source / 1000000000UL );
    out_date->nanosecond   = ( source % 1000000000UL );
}


bool axk_date_to_time( struct axk_date_t* in_date, struct axk_time_t* out_time )
{
    uint32_t leap_days = 0;
    uint32_t leap_secs = 0;

    if( in_date->year < 2000 || in_date->year > 4000 ) { return false; }

    // Ensure 'day' and 'month' are clamped to 1 on its lower bounds
    in_date->day       = in_date->day < 1 ? 1 : in_date->day;
    in_date->month     = in_date->month < 1 ? 1 : in_date->month;

    for( uint32_t i = 2000; i < in_date->year; i += 4 )
    {
        leap_days++;
    }

    if( in_date->year > 2005 ) { leap_secs++; }
    if( in_date->year > 2008 || ( in_date->year == 2008 && in_date->month > 6 ) ) { leap_secs++; }
    if( in_date->year > 2015 || ( in_date->year == 2015 && in_date->month > 6 ) ) { leap_secs++; }
    if( in_date->year > 2016 ) { leap_secs++; }

    // Now, we can boil everything down to seconds
    uint64_t total_secs = (uint64_t) in_date->second;
    total_secs += ( (uint64_t) in_date->minute * 60UL );
    total_secs += ( (uint64_t) in_date->hour * 3600UL );
    total_secs += ( (uint64_t) ( in_date->day - 1 ) * 86400UL );
    
    // We need to calculate the number of days in the current year, since the begining, taking into account leap years
    uint32_t days_this_year     = 0U;
    bool b_leap_year            = ( ( ( in_date->year - 2000 ) % 4 ) == 0 );
    const uint8_t* year_info          = b_leap_year ? _leap_year : _non_leap_year;

    for( uint8_t i = 0; i < ( in_date->month - 1 ); i++ )
    {
        days_this_year += (uint32_t)year_info[ i ];
    }

    total_secs += ( (uint64_t)days_this_year * 86400UL );

    // Now, we just need to calculate the numebr of seconds from 2000 to the start of this year
    // We have the leap second count, and the leap day count
    total_secs += ( ( ( ( (uint64_t)in_date->year - 2000UL ) * 365UL ) + leap_days ) * 86400UL );
    total_secs += leap_secs;

    out_time->raw = ( total_secs * 1000000000UL ) + in_date->nanosecond;
    return true;
}


int axk_time_compare( struct axk_time_t* a, struct axk_time_t* b, enum axk_time_unit_t unit )
{
    // Convert the times to the specified unit
    uint64_t a_unit = axk_time_convert( a->raw, AXK_TIME_UNIT_NANOSECOND, unit );
    uint64_t b_unit = axk_time_convert( b->raw, AXK_TIME_UNIT_NANOSECOND, unit );

    if( a_unit > b_unit )
    {
        return 1;
    }
    else
    {
        return -1;
    }
}


int axk_time_compare_d( struct axk_time_t* a, struct axk_time_t* b, enum axk_time_unit_t unit, uint64_t delta )
{
    // Convert the times to the specified unit
    uint64_t a_unit = axk_time_convert( a->raw, AXK_TIME_UNIT_NANOSECOND, unit );
    uint64_t b_unit = axk_time_convert( b->raw, AXK_TIME_UNIT_NANOSECOND, unit );

    if( a_unit > b_unit )
    {
        if( ( a_unit - b_unit ) <= delta ) { return 0; }
        return 1;
    }
    else
    {
        if( ( b_unit - a_unit ) <= delta ) { return 0; }
        return -1;
    }
}


bool axk_time_add_duration( struct axk_time_t* in_time, struct axk_duration_t* in_dur )
{
    // We need to convert the duration to nanoseconds and add it to the raw system time
    uint64_t raw_duration = axk_time_convert( in_dur->value, in_dur->unit, AXK_TIME_UNIT_NANOSECOND );
    if( raw_duration == 0UL ) { return true; }

    if( raw_duration + in_time->raw <= in_time->raw ) { return false; }
    in_time->raw += raw_duration;
    return true;
}


bool axk_time_subtract_duration( struct axk_time_t* in_time, struct axk_duration_t* in_dur )
{
    uint64_t raw_duration = axk_time_convert( in_dur->value, in_dur->unit, AXK_TIME_UNIT_NANOSECOND );
    if( raw_duration == 0UL ) { return true; }

    if( raw_duration > in_time->raw ) { return false; }
    in_time->raw -= raw_duration;
    return true;
}


void axk_time_get_duration( struct axk_time_t* a, struct axk_time_t* b, enum axk_time_unit_t unit, struct axk_duration_t* out_dur )
{
    // First calculate the nanosecond difference
    uint64_t diff = a->raw > b->raw ? a->raw - b->raw : b->raw - a->raw;

    // Convert to the desired unit
    out_dur->value  = axk_time_convert( diff, AXK_TIME_UNIT_NANOSECOND, unit );
    out_dur->unit   = unit;
}


int axk_date_compare( struct axk_date_t* a, struct axk_date_t* b )
{
    if( a->year > b->year )                 { return 1; }
    if( a->year < b->year )                 { return -1; }
    if( a->month > b->year )                { return 1; }
    if( a->month < b->year )                { return -1; }
    if( a->day > b->day )                   { return 1; }
    if( a->day < b->day )                   { return -1; }
    if( a->hour > b->hour )                 { return 1; }
    if( a->hour < b->hour )                 { return -1; }
    if( a->minute > b->minute )             { return 1; }
    if( a->minute < b->minute )             { return -1; }
    if( a->second > b->second )             { return 1; }
    if( a->second < b->second )             { return -1; }
    if( a->nanosecond > b->nanosecond )     { return 1; }
    if( a->nanosecond < b->nanosecond )     { return -1; }

    return 0;
}


bool axk_date_is_valid_time( struct axk_date_t* in_date )
{
    return( in_date->year >= 2000 && in_date->year <= 4000 );
}


bool axk_date_add( struct axk_date_t* in_date, struct axk_duration_t* in_dur )
{
    // TODO: Dont use system time.. its going to lead to issues if were out of the 'system time' range
    struct axk_time_t t;
    if( !axk_date_to_time( in_date, &t ) ) { return false; }
    if( !axk_time_add_duration( &t, in_dur ) ) { return false; }
    axk_time_to_date( &t, in_date );
}


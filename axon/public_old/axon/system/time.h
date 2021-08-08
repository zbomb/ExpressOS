/*==============================================================
    Axon Kernel - Time Structures
    2021, Zachary Berry
    axon/public/axon/system/time.h
==============================================================*/

#pragma once
#include "axon/config.h"

/*
    axk_time_unit_t (ENUM)
    * Different units of measurment for time
*/
enum axk_time_unit_t
{
    AXK_TIME_UNIT_NANOSECOND    = 0,
    AXK_TIME_UNIT_MICROSECOND   = 1,
    AXK_TIME_UNIT_MILLISECOND   = 2,
    AXK_TIME_UNIT_SECOND        = 3,
    AXK_TIME_UNIT_MINUTE        = 4,
    AXK_TIME_UNIT_HOUR          = 5
};

/*
    axk_time_t
    * Stores the number of nanoseconds since the year 2000
*/
struct axk_time_t
{
    uint64_t raw;
};

/*
    axk_date_t
    * Stores the current date/time
    * Formatted as 24 hour days, UTC
*/
struct axk_date_t
{
    uint32_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint32_t nanosecond;
};

/*
    axk_duration_t
    * Stores a duration of time, since an arbitrary point
    * Uses a 'unit' part, and a 'value'
*/
struct axk_duration_t
{
    enum axk_time_unit_t unit;
    uint64_t value;
};

/*
    axk_time_range_t
    * Stores a range of time, the begin and end of the range stored as axk_time_t
*/
struct axk_time_range_t
{
    struct axk_time_t begin;
    struct axk_time_t end;
};


/*
    axk_time_convert_duration
    * Converts a duration to a new unit
*/
void axk_time_convert_duration( struct axk_duration_t* in_value, enum axk_time_unit_t out_unit, struct axk_duration_t* out_value );

/*
    axk_time_convert
    * Converts a time and unit, to another unit
*/
uint64_t axk_time_convert( uint64_t in_value, enum axk_time_unit_t in_unit, enum axk_time_unit_t out_unit );

/*
    axk_time_to_date
    * Converts a system time to date
*/
void axk_time_to_date( struct axk_time_t* in_time, struct axk_date_t* out_date );

/*
    axk_time_from_date
    * Converts a date to a system time structure
    * Can fail! If the date is out of the system time range (0xFFFFFFFFFFFFFFFF nanoseconds since year 2000 is the max)
*/
bool axk_date_to_time( struct axk_date_t* in_date, struct axk_time_t* out_time );

/*
    axk_time_compare
    * Compares two times against each other
    * Uses the specified unit as the precision
    * Returns 0 IFF 'a' is equal to 'b' with the specified precision
    * If 'a' < 'b', returns a negative number
    * If 'a' > 'b', returns a positive number
*/
int axk_time_compare( struct axk_time_t* a, struct axk_time_t* b, enum axk_time_unit_t unit );

/*
    axk_time_compare_d
    * Compares two times against each other
    * Uses the specified unit as precision
    * Includes a delta, allowing the times to be within a range and still pass comparison
    * Returns 0 IFF 'a' is within [unit] [delta] of 'b'
    * If 'a' < 'b' - [uint delta], returns a negative number
    * If 'a' > 'b' - [unit delta], returns a positive number
*/
int axk_time_compare_d( struct axk_time_t* a, struct axk_time_t* b, enum axk_time_unit_t unit, uint64_t delta );

/*
    axk_time_add_duration
    * Adds a duration to a system time
    * Returns false if the addition overflows
*/
bool axk_time_add_duration( struct axk_time_t* in_time, struct axk_duration_t* in_dur );

/*
    axk_time_subtract_duration
    * Subtracts a duration from a system time
    * Returns false if the subtraction underflows
*/
bool axk_time_subtract_duration( struct axk_time_t* in_time, struct axk_duration_t* in_dur );

/*
    axk_time_get_duration
    * Calculates the duration between two system time objects
    * The result is converted to the specified unit
    * The result is the absolute value of the subtraction (a-b)
*/
void axk_time_get_duration( struct axk_time_t* a, struct axk_time_t* b, enum axk_time_unit_t unit, struct axk_duration_t* out_dur );

/*
    axk_date_compare
    * Compares two system date objects
    * Returns 0 IFF a and b are the same
    * Returns a negative number, if a < b
    * Returns a positive number, if a > b
*/
int axk_date_compare( struct axk_date_t* a, struct axk_date_t* b );

/*
    axk_date_is_valid_time
    * Checks if a system date, is a valid system time
    * i.e. is within the valid system time range
*/
bool axk_date_is_valid_time( struct axk_date_t* in_date );

/*
    axk_date_add
    * Adds a duration to a system date structure
*/
bool axk_date_add( struct axk_date_t* in_date, struct axk_duration_t* in_dur );

/*
    axk_time_get
    * Gets the current system time
*/
void axk_time_get( struct axk_time_t* out_time );

/*
    axk_time_get_since_boot
    * Gets the amount of time that has passed since boot in nanoseconds
*/
uint64_t axk_time_get_since_boot( void );

/*
    axk_date_get
    * Gets the current date
*/
void axk_date_get( struct axk_date_t* out_date );
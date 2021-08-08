/*==============================================================
    Axon Kernel - Time Keeping (x86 Specific Code)
    2021, Zachary Berry
    axon/source/arch_x86/time_x86.c
==============================================================*/

#ifdef _X86_64_
#include "axon/system/time_private.h"
#include "axon/system/timers.h"
#include "axon/arch_x86/acpi_info.h"
#include "axon/arch_x86/util.h"
#include "axon/library/spinlock.h"
#include "axon/config.h"

/*
    Constants
*/
#define CMOS_ADDRESS_REGISTER   0x70
#define CMOS_DATA_REGISTER      0x71
#define RTC_REGISTER_SECONDS    0x00
#define RTC_REGISTER_MINUTES    0x02
#define RTC_REGISTER_HOURS      0x04
#define RTC_REGISTER_DAYS       0x07
#define RTC_REGISTER_MONTHS     0x08
#define RTC_REGISTER_YEARS      0x09
#define RTC_REGISTER_UPDATING   0x0A
#define RTC_REGISTER_FORMAT     0x0B
#define RTC_MAX_READ_ATTEMPTS   50

/*
    State
*/
static struct axk_spinlock_t g_lock;
static uint8_t g_century_register;

/*
    Function Implementations
*/
bool axk_time_init_persistent_clock( void )
{
    struct axk_x86_acpi_info_t* ptr_acpi = axk_x86_acpi_get();
    if( ptr_acpi != NULL && ptr_acpi->fadt != NULL )
    {
        g_century_register = ptr_acpi->fadt->century;
    }
    else
    {
        g_century_register = 0;
    }

    axk_spinlock_init( &g_lock );
    return true;
}


uint8_t read_cmos( uint8_t reg )
{
    axk_x86_outb( CMOS_ADDRESS_REGISTER, reg );
    return axk_x86_inb( CMOS_DATA_REGISTER );
}


bool axk_time_read_persistent_clock( struct axk_date_t* out_date, uint64_t* out_counter_sync )
{
    if( out_date == NULL ) { return false; }

    // Acquire lock
    axk_spinlock_acquire( &g_lock );

    // Next we need to wait for an RTC update before reading to ensure we get accurate values
    while( ( read_cmos( RTC_REGISTER_UPDATING ) & 0x80 ) != 0 )
    {
        __asm__( "pause" );
    }

    // Now we can read the 'standard' registers
    uint8_t sec    = read_cmos( RTC_REGISTER_SECONDS );
    uint8_t min    = read_cmos( RTC_REGISTER_MINUTES );
    uint8_t hr     = read_cmos( RTC_REGISTER_HOURS );
    uint8_t day    = read_cmos( RTC_REGISTER_DAYS );
    uint8_t mth    = read_cmos( RTC_REGISTER_MONTHS );
    uint8_t yr     = read_cmos( RTC_REGISTER_YEARS );

    // Check if we found a 'century register' in ACPI, since its non-standard, it might not exist
    uint8_t cnt = g_century_register > 0x00 ? read_cmos( g_century_register ) : 0x00;

    // We will attempt to read the values again and we want to check if they match the values we just read
    // This is because reading CMOS can be buggy, so we want to be absolutley sure we got a good read 
    uint8_t psec, pmin, phr, pday, pmth, pyr, pcnt, fmt;
    uint32_t t = 0;

    do
    {
        // Give up after 'RTC_MAX_READ_ATTEMPTS' tries
        if( t++ > RTC_MAX_READ_ATTEMPTS ) { axk_spinlock_release( &g_lock ); return false; }

        psec = sec; pmin = min; phr = hr; pday = day; pmth = mth; pyr = yr; pcnt = cnt;

        // Ensure the RTC isnt being updated
        while( ( read_cmos( RTC_REGISTER_UPDATING ) & 0x80 ) != 0 )
        {
            __asm__( "pause" );
        }

        *out_counter_sync = axk_timer_get_counter_value( axk_timer_get_counter() );

        // Re-read the registers
        sec    = read_cmos( RTC_REGISTER_SECONDS );
        min    = read_cmos( RTC_REGISTER_MINUTES );
        hr     = read_cmos( RTC_REGISTER_HOURS );
        day    = read_cmos( RTC_REGISTER_DAYS );
        mth    = read_cmos( RTC_REGISTER_MONTHS );
        yr     = read_cmos( RTC_REGISTER_YEARS );

        cnt = g_century_register > 0x00 ? read_cmos( g_century_register ) : 0x00;
    }
    while( sec != psec || min != pmin || hr != phr || day != pday || mth != pmth || yr != pyr || cnt != pcnt );

    // Now that we have consistent readings, lets check what format theyre in and prepare the output values
    fmt = read_cmos( RTC_REGISTER_FORMAT );
    uint8_t ohr = hr;

    if( ( fmt & 0x04 ) == 0x00 )
    {
        out_date->second   = ( sec & 0x0F ) + ( ( sec / 16 ) * 10 );
        out_date->minute   = ( min & 0x0F ) + ( ( min / 16 ) * 10 );
        out_date->hour     = ( hr & 0x0F ) + ( ( ( hr & 0x70 ) / 16 ) * 10 ) + ( hr & 0x80 );
        out_date->day      = ( day & 0x0F ) + ( ( day / 16 ) * 10 );
        out_date->month    = ( mth & 0x0F ) + ( ( mth / 16 ) * 10 );
        out_date->year     = ( yr & 0x0F ) + ( ( yr / 16 ) * 10 );

        if( g_century_register != 0 )
        {
            cnt = ( cnt & 0x0F ) + ( ( cnt / 16 ) * 10 );
        }

        ohr = out_date->hour;
    }

    if( ( fmt & 0x02 ) == 0x00 )
    {
        int ampm = ohr & 0x80;
        ohr = ( ohr & 0x7F );
        if( ohr == 12 ) { ohr = 0; }
        if( ampm ) { ohr += 12; }
    }

    out_date->hour = ohr;

    if( g_century_register != 0 )
    {
        out_date->year += cnt * 100;
    }
    else
    {
        // Basically just guess the year
        out_date->year += ( AXK_DEFAULT_YEAR / 100 ) * 100;
        if( out_date->year < AXK_DEFAULT_YEAR ) { out_date->year += 100; }
    }

    axk_spinlock_release( &g_lock );
    return true;
}


bool axk_time_write_persistent_clock( struct axk_date_t* in_date )
{
    return false;
}

#endif
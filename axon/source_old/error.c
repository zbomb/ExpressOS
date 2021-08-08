/*==============================================================
    Axon Kernel - Error System
    2021, Zachary Berry
    axon/source/error.c
==============================================================*/

#include "axon/config.h"
#define ERR_MAX_STR 254


void axk_error_reset( struct axk_error_t* err, uint32_t err_code )
{
    memset( err, 0, sizeof( struct axk_error_t ) );
    err->code = err_code;
}

static inline void _write_char( struct axk_error_t* err, char c )
{
    if( err->message_len < ERR_MAX_STR ) { err->message[ err->message_len++ ] = c; err->message[ err->message_len ] = '\0'; }
}

void axk_error_write_str( struct axk_error_t* err, const char* str )
{
    size_t str_len = strlen( str );
    uint8_t remaining = ERR_MAX_STR - err->message_len;
    str_len = str_len > remaining ? remaining : str_len;

    for( size_t i = 0; i < str_len; i++ )
    {
        err->message[ err->message_len++ ] = str[ i ];
    }

    if( str_len > 0UL ) { err->message[ err->message_len ] = '\0'; }
}


static void _print_hex_byte( struct axk_error_t* err, uint8_t in_byte, bool* b_print_lz )
{
    static const char lookup_table[ 16 ] = 
    {
        '0',
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        'A',
        'B',
        'C',
        'D',
        'E',
        'F'
    };

    uint8_t high = ( in_byte & 0xF0 ) >> 4;
    uint8_t low = ( in_byte & 0x0F );

    if( *b_print_lz || high > 0 )
    {
        *b_print_lz = true;
        _write_char( err, lookup_table[ high ] );
        _write_char( err, lookup_table[ low ] );
    }
    else
    {
        if( *b_print_lz || low > 0 )
        {
            *b_print_lz = true;
            _write_char( err, lookup_table[ low ] );
        }
    }
}


void axk_error_write_u32( struct axk_error_t* err, uint32_t num )
{
    if( num == 0U )
    {
        _write_char( err, '0' );
    }
    else
    {
        // Loop through each digit, get its value and print the corresponding character
        // Max value of a uint32_t has 10 digits in base 10
        uint32_t place_value = 1000000000;
        bool b_zero = true;
        uint8_t index = 0;

        for( int i = 9; i >= 0; i-- )
        {
            if( b_zero && place_value > num )
            {
                place_value /= 10;
                continue;
            }

            b_zero = false;
            uint32_t digit = num / place_value;
            _write_char( err, (char)( digit + 0x30 ) );

            num -= digit * place_value;
            place_value /= 10;
        }
    }
}


void axk_error_write_h32( struct axk_error_t* err, uint32_t num, bool b_print_lz )
{
    // Write the hex header
    _write_char( err, '0' );
    _write_char( err, 'x' );

    // Write each byte in hex
    _print_hex_byte( err, ( num & 0xFF000000 ) >> 24, &b_print_lz );
    _print_hex_byte( err, ( num & 0x00FF0000 ) >> 16, &b_print_lz );
    _print_hex_byte( err, ( num & 0x0000FF00 ) >> 8, &b_print_lz );
    _print_hex_byte( err, ( num & 0x000000FF ), &b_print_lz );

    // If we trimmed leading zeros, and the string was empty, we need to at laest print one zero digit
    if( !b_print_lz )
    {
        _write_char( err, '0' );
    }
}


void axk_error_write_u64( struct axk_error_t* err, uint64_t num )
{
    if( num == 0UL )
    {
        _write_char( err, '0' );
    }
    else if( num <= 0xFFFFFFFF )
    {
        axk_error_write_u32( err, (uint32_t)num );
    }
    else
    {
        uint64_t place_value = 10000000000000000000UL;
        bool b_zero = true;
        uint8_t index = 0;

        for( int i = 19; i >= 0; i-- )
        {
            if( b_zero && place_value > num )
            {
                place_value /= 10UL;
                continue;
            }

            b_zero = false;
            uint64_t digit = num / place_value;
            _write_char( err, (char)( digit + 0x30 ) );

            num -= digit * place_value;
            place_value /=  10UL;
        }
    }
}


void axk_error_write_h64( struct axk_error_t* err, uint64_t num, bool b_print_lz )
{
    // Write the hex header
    _write_char( err, '0' );
    _write_char( err, 'x' );

    // Write each byte in hex
    _print_hex_byte( err, ( num & 0xFF00000000000000UL ) >> 56, &b_print_lz );
    _print_hex_byte( err, ( num & 0x00FF000000000000UL ) >> 48, &b_print_lz );
    _print_hex_byte( err, ( num & 0x0000FF0000000000UL ) >> 40, &b_print_lz );
    _print_hex_byte( err, ( num & 0x000000FF00000000UL ) >> 32, &b_print_lz );
    _print_hex_byte( err, ( num & 0x00000000FF000000UL ) >> 24, &b_print_lz );
    _print_hex_byte( err, ( num & 0x0000000000FF0000UL ) >> 16, &b_print_lz );
    _print_hex_byte( err, ( num & 0x000000000000FF00UL ) >> 8, &b_print_lz );
    _print_hex_byte( err, ( num & 0x00000000000000FFUL ), &b_print_lz );

    // If we trimmed leading zeros, and the string was empty, we need to at laest print one zero digit
    if( !b_print_lz )
    {
        _write_char( err, '0' );
    }
}



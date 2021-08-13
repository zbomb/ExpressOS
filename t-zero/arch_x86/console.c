/*==============================================================
    T-0 Bootloader - Console Functions
    2021, Zachary Berry
    t-zero/source/console.c
==============================================================*/

#include "console.h"
#include "util.h"

/*
    Globals
*/
extern EFI_SYSTEM_TABLE* efi_table;

/*
    Function Implementations
*/
void tzero_console_prints16( CHAR16* str )
{
    efi_table->ConOut->OutputString( efi_table->ConOut, str == NULL ? L"NULL" : str );
}


void tzero_console_prints8( const char* str )
{
    size_t len = str == NULL ? 0UL : strlen( str );
    if( len == 0UL ) 
    { 
        efi_table->ConOut->OutputString( efi_table->ConOut, L"NULL" );
    }
    else
    {
        // Determine length of the string, copy into a u-16 formatted string
        uint16_t* u16_str = (uint16_t*) tzero_alloc( ( len + 1UL ) * 2UL );

        for( size_t i = 0; i < len; i++ )
        {
            u16_str[ i ] = str[ i ];
        }
        u16_str[ len ] = 0x00;

        // Print UTF-16 string as normal
        efi_table->ConOut->OutputString( efi_table->ConOut, (CHAR16*) u16_str );
        tzero_free( u16_str );
    }
}


void tzero_console_clear( void )
{
    efi_table->ConOut->ClearScreen( efi_table->ConOut );
}


void tzero_console_printnl( void )
{
    efi_table->ConOut->OutputString( efi_table->ConOut, L"\r\n" );
}


void tzero_console_set_color( uint8_t foreground, uint8_t background )
{
    efi_table->ConOut->SetAttribute( efi_table->ConOut, foreground | ( background << 4 ) );
}


void tzero_console_set_foreground( uint8_t color )
{
    efi_table->ConOut->SetAttribute( efi_table->ConOut, color | ( efi_table->ConOut->Mode->Attribute & 0b1110000 ) );
}


void tzero_console_set_background( uint8_t color )
{
    efi_table->ConOut->SetAttribute( efi_table->ConOut, ( efi_table->ConOut->Mode->Attribute & 0b1111 ) | ( color << 4 ) );
}


void tzero_console_printu32( uint32_t num )
{
    if( num == 0U )
    {
        efi_table->ConOut->OutputString( efi_table->ConOut, L"0" );
    }
    else
    {
        // Loop through each digit, get its value and print the corresponding character
        // Max value of a uint32_t has 10 digits in base 10
        uint32_t place_value = 1000000000;
        bool b_zero = true;
        uint16_t str_data[ 11 ];
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

            str_data[ index++ ] = (uint16_t)( digit + 0x30 );

            num -= digit * place_value;
            place_value /= 10;
        }
        
        // Write null terminator character at the end of the string data
        str_data[ index ] = 0x00;
        efi_table->ConOut->OutputString( efi_table->ConOut, (CHAR16*) str_data );
    }
}


void tzero_console_printu64( uint64_t num )
{
    if( num == 0UL )
    {
        efi_table->ConOut->OutputString( efi_table->ConOut, L"0" );
    }
    else if( num <= 0xFFFFFFFF )
    {
        tzero_console_printu32( (uint32_t) num );
    }
    else
    {
        uint64_t place_value = 10000000000000000000UL;
        bool b_zero = true;
        uint16_t str_data[ 21 ];
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

            str_data[ index++ ] = (uint16_t)( digit + 0x30 );

            num -= digit * place_value;
            place_value /=  10UL;
        }

        // Write null terminator and print string
        str_data[ index ] = 0x00;
        efi_table->ConOut->OutputString( efi_table->ConOut, (CHAR16*) str_data );
    }
}


uint8_t _print_hex_byte( uint8_t in_byte, uint16_t* out_chars, bool* b_print_lz )
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
        out_chars[ 0 ] = lookup_table[ high ];
        out_chars[ 1 ] = lookup_table[ low ];

        return 2;
    }
    else
    {
        if( *b_print_lz || low > 0 )
        {
            *b_print_lz = true;
            out_chars[ 0 ] = lookup_table[ low ];

            return 1;
        }
        else
        {
            return 0;
        }
    }
}


void tzero_console_printh32( uint32_t num, bool b_leading_zeros )
{
    uint16_t str_data[ 11 ];
    uint8_t index = 0;

    // Write the hex header
    str_data[ index++ ] = (uint16_t)( '0' );
    str_data[ index++ ] = (uint16_t)( 'x' );

    // Write each byte in hex
    index += _print_hex_byte( ( num & 0xFF000000 ) >> 24, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x00FF0000 ) >> 16, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x0000FF00 ) >> 8, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x000000FF ), str_data + index, &b_leading_zeros );

    // If we trimmed leading zeros, and the string was empty, we need to at laest print one zero digit
    if( !b_leading_zeros )
    {
        str_data[ index++ ] = (uint16_t)( '0' );
    }

    // Write null terminator
    str_data[ index ] = 0x00;

    // Print the string
    efi_table->ConOut->OutputString( efi_table->ConOut, (CHAR16*) str_data );
}


void tzero_console_printh64( uint64_t num, bool b_leading_zeros )
{
    uint16_t str_data[ 19 ];
    uint8_t index = 0;

    // Write the hex header
    str_data[ index++ ] = (uint16_t)( '0' );
    str_data[ index++ ] = (uint16_t)( 'x' );

    // Write each byte in hex
    index += _print_hex_byte( ( num & 0xFF00000000000000 ) >> 56, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x00FF000000000000 ) >> 48, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x0000FF0000000000 ) >> 40, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x000000FF00000000 ) >> 32, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x00000000FF000000 ) >> 24, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x0000000000FF0000 ) >> 16, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x000000000000FF00 ) >> 8, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x00000000000000FF ), str_data + index, &b_leading_zeros );

    // If we trimmed leading zeros, and the string was empty, we need to at laest print one zero digit
    if( !b_leading_zeros )
    {
        str_data[ index++ ] = (uint16_t)( '0' );
    }

    // Write null terminator
    str_data[ index ] = 0x00;

    // Print the string
    efi_table->ConOut->OutputString( efi_table->ConOut, (CHAR16*) str_data );
}
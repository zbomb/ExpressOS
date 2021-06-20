/*==============================================================
    Axon Kernel - Debug Printing Library
    2021, Zachary Berry
    axon/source/debug_print.c
==============================================================*/

#include "axon/debug_print.h"
#include "axon/library/spinlock.h"

/*
    Constants and types
*/
static const uint32_t TERMINAL_WIDTH    = 80;
static const uint32_t TERMINAL_HEIGHT   = 25;
static const uint32_t TERMINAL_MAX_STR_LEN = 0xFFFFFFFF;
static const uint32_t TERMINAL_TAB_SIZE = 4;

struct terminal_char_t
{
    uint8_t code;
    uint8_t fg : 4;
    uint8_t bg : 3;
};

// This is a pointer to the memory location where we need to place our TerminalChars for them to be displayed on screen
const void* ptr_terminal_buffer = (void*) 0xFFFFFFFF800B8000;

/*
    Terminal state
*/
uint32_t terminal_x         = 0;
uint32_t terminal_y         = 0;
uint8_t terminal_bg         = TC_BLACK;
uint8_t terminal_fg         = TC_LIGHT_GRAY;
bool terminal_use_color     = true;
struct axk_spinlock_t terminal_lock;

/*
    Function Definitions
*/

void axk_terminal_lock( void )
{
    axk_spinlock_acquire( &terminal_lock );
}

void axk_terminal_unlock( void )
{
    axk_spinlock_release( &terminal_lock );
}

void axk_terminal_clear_row( uint32_t row )
{
    // Validate row number
    if( row > TERMINAL_HEIGHT ) { return; }

    // Get the offset of target row
    uint32_t row_offset = row * TERMINAL_WIDTH * 2;
    uint32_t row_end = row_offset + ( TERMINAL_WIDTH * 2 );

    for( uint32_t i = row_offset; i < row_end; i += 2 )
    {
        uint8_t* ptr = (uint8_t*)( ptr_terminal_buffer ) + i;
        *( ptr++ ) = 0;
        *ptr = 0;
    }
}

void axk_terminal_clear_range( uint32_t start_col, uint32_t start_row, uint32_t end_col, uint32_t end_row )
{
    // Validate parameters
    if( start_row > end_row || start_col > end_col || end_row > TERMINAL_HEIGHT || end_col > TERMINAL_WIDTH ) { return; }

    uint32_t start_offset    = ( ( start_row * TERMINAL_WIDTH ) + start_col ) * 2;
    uint32_t end_offset      = ( ( end_row * TERMINAL_WIDTH ) + end_col ) * 2;

    for( uint32_t i = start_offset; i < end_offset; i += 2 )
    {
        uint8_t* ptr = (uint8_t*)( ptr_terminal_buffer ) + i;
        *( ptr++ ) = 0;
        *ptr = 0;
    }
}

void axk_terminal_clear( void )
{
    axk_terminal_clear_range( 0, 0, TERMINAL_WIDTH, TERMINAL_HEIGHT );
    terminal_x = 0;
    terminal_y = 0;
}

void axk_terminal_handle_full( void )
{
    // When the terminal gets full, this function is called to deal with it
    // What we will do, is shift everything up one line and clear the bottom row
    // We can then set the terminal position to that last row
    for( uint32_t row = 0; row < TERMINAL_HEIGHT - 1; row++ )
    {
        for( uint32_t col = 0; col < TERMINAL_WIDTH; col++ )
        {
            uint8_t* ptr_this = (uint8_t*)( ptr_terminal_buffer ) + ( row * TERMINAL_WIDTH * 2 ) + ( col * 2 );
            uint8_t* ptr_next = ptr_this + ( TERMINAL_WIDTH * 2 );

            ptr_this[ 0 ] = ptr_next[ 0 ];
            if( terminal_use_color )
            {
                ptr_this[ 1 ] = ptr_next[ 1 ];
            }
        }
    }

    uint8_t* ptr_last = (uint8_t*)( ptr_terminal_buffer ) + ( TERMINAL_WIDTH * ( TERMINAL_HEIGHT - 1 ) * 2 );
    for( uint32_t i = 0; i < TERMINAL_WIDTH; i++ )
    {
        uint8_t* p = ptr_last + ( i * 2 );
        p[ 0 ] = 0;
        if( terminal_use_color )
        {
            p[ 1 ] = 0;
        }
    }

    terminal_y = TERMINAL_HEIGHT - 1;
    terminal_x = 0;

}

void axk_terminal_printc( char in )
{
    // Check for new line characters
    if( in == '\n' )
    {
        axk_terminal_printnl();
        return;
    }

    switch( in )
    {
        case '\n':
        axk_terminal_printnl();
        break;

        case '\t':
        axk_terminal_printtab();
        break;

        default:
        {
            // Get current memory offset and write the character and current color
            uint8_t* ptr = (uint8_t*)( ptr_terminal_buffer ) + ( terminal_y * TERMINAL_WIDTH * 2 ) + ( terminal_x * 2 );
            ptr[ 0 ] = (uint8_t) in;

            if( terminal_use_color )
            {
                ptr[ 1 ] = ( terminal_fg | ( terminal_bg & 0b0000111 ) << 4 );
            }

            // Incrememnt our position in the terminal and check for new line
            if( ++terminal_x >= TERMINAL_WIDTH )
            {
                terminal_x = 0;
                
                // Check if we ran out of space and need to shift the terminal up one line
                if( ++terminal_y >= TERMINAL_HEIGHT )
                {
                    axk_terminal_handle_full();
                }
            }
        }
        break;
    }
}

void axk_terminal_prints( const char* str )
{
    // Printing a string is just looping through and printing indivisual characters for now
    // We have a max string size, if we end up looping longer, we assume an error
    uint32_t counter = 0;
    const char* ptr = str;

    while( *ptr != '\0' )
    {
        counter++;
        if( counter >= TERMINAL_MAX_STR_LEN ) { break;}

        axk_terminal_printc( *( ptr++ ) );
    }
}

void axk_terminal_printu32( uint32_t num )
{
    if( num == 0U )
    {
        axk_terminal_printc( '0' );
    }
    else
    {
        // Loop through each digit, get its value and print the corresponding character
        // Max value of a uint32_t has 10 digits in base 10
        uint32_t place_value = 1000000000;
        bool b_zero = true;

        for( int i = 9; i >= 0; i-- )
        {
            if( b_zero && place_value > num )
            {
                place_value /= 10;
                continue;
            }

            b_zero = false;
            uint32_t digit = num / place_value;

            axk_terminal_printc( (uint8_t)digit + 0x30 );

            num -= digit * place_value;
            place_value /= 10;
        }
        
    }
}

void axk_terminal_printu64( uint64_t num )
{
    if( num == 0UL )
    {
        axk_terminal_printc( '0' );
    }
    else if( num <= 0xFFFFFFFF )
    {
        axk_terminal_printu32( (uint32_t) num );
    }
    else
    {
        uint64_t place_value = 10000000000000000000UL;
        bool b_zero = true;
        for( int i = 19; i >= 0; i-- )
        {
            if( b_zero && place_value > num )
            {
                place_value /= 10UL;
                continue;
            }

            b_zero = false;
            uint64_t digit = num / place_value;

            axk_terminal_printc( (uint8_t)digit + 0x30 );

            num -= digit * place_value;
            place_value /=  10UL;
        }
    }
}

void axk_terminal_printi32( int32_t num )
{
    if( num < 0 )
    {
        axk_terminal_printc( '-' );
        axk_terminal_printu32( (uint32_t)( -num ) );
    }
    else
    {
        axk_terminal_printu32( (uint32_t) num );
    }

}

void axk_terminal_printi64( int64_t num )
{
    if( num < 0L )
    {
        axk_terminal_printc( '-' );
        axk_terminal_printu64( (uint64_t)( -num ) );
    }
    else
    {
        axk_terminal_printu64( (uint64_t) num );
    }
}

void _impl_print_hex( uint8_t in, bool* b_zero )
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

    *b_zero = *b_zero ? in == 0 : false;
    if( *b_zero ) { return; }

    axk_terminal_printc( in > 15 ? 'F' : lookup_table[ in ] );
}

void _impl_print_hex_header( void )
{
    axk_terminal_printc( '0' );
    axk_terminal_printc( 'x' );
}

void axk_terminal_printh8( uint8_t in )
{
    axk_terminal_printh8_lz( in, false );
}

void axk_terminal_printh8_lz( uint8_t in, bool b_leading_zeros )
{
    uint8_t lb = in & 0x0F;
    uint8_t hb = ( in & 0xF0 ) >> 4;
    bool b_zero = !b_leading_zeros;
    
    _impl_print_hex_header();
    _impl_print_hex( hb, &b_zero ); 
    _impl_print_hex( lb, &b_zero );

    if( b_zero ) { axk_terminal_printc( '0' ); }
}

void axk_terminal_printh16( uint16_t in )
{
    axk_terminal_printh16_lz( in, false );
}

void axk_terminal_printh16_lz( uint16_t in, bool b_leading_zeros )
{
    uint8_t l_lb = in & 0x000F;
    uint8_t l_hb = ( in & 0x00F0 ) >> 4;
    uint8_t h_lb = ( in & 0x0F00 ) >> 8;
    uint8_t h_hb = ( in & 0xF000 ) >> 12;
    bool b_zero = !b_leading_zeros;

    _impl_print_hex_header();
    _impl_print_hex( h_hb, &b_zero );
    _impl_print_hex( h_lb, &b_zero );
    _impl_print_hex( l_hb, &b_zero );
    _impl_print_hex( l_lb, &b_zero );

    if( b_zero ) { axk_terminal_printc( '0' ); }
}

void axk_terminal_printh32( uint32_t in )
{
    axk_terminal_printh32_lz( in, false );
}

void axk_terminal_printh32_lz( uint32_t in, bool b_leading_zeros )
{
    uint8_t l_l_lb = in & 0x0000000F;
    uint8_t l_l_hb = ( in & 0x000000F0 ) >> 4;
    uint8_t l_h_lb = ( in & 0x00000F00 ) >> 8;
    uint8_t l_h_hb = ( in & 0x0000F000 ) >> 12;
    uint8_t h_l_lb = ( in & 0x000F0000 ) >> 16;
    uint8_t h_l_hb = ( in & 0x00F00000 ) >> 20;
    uint8_t h_h_lb = ( in & 0x0F000000 ) >> 24;
    uint8_t h_h_hb = ( in & 0xF0000000 ) >> 28;
    bool b_zero = !b_leading_zeros;

    _impl_print_hex_header();
    _impl_print_hex( h_h_hb, &b_zero );
    _impl_print_hex( h_h_lb, &b_zero );
    _impl_print_hex( h_l_hb, &b_zero );
    _impl_print_hex( h_l_lb, &b_zero );
    _impl_print_hex( l_h_hb, &b_zero );
    _impl_print_hex( l_h_lb, &b_zero );
    _impl_print_hex( l_l_hb, &b_zero );
    _impl_print_hex( l_l_lb, &b_zero );

    if( b_zero ) { axk_terminal_printc( '0' ); }
}

void axk_terminal_printh64( uint64_t in )
{
    axk_terminal_printh64_lz( in, false );
}

void axk_terminal_printh64_lz( uint64_t in, bool b_leading_zeros )
{
    uint8_t l_l_lb = in & 0x000000000000000F;
    uint8_t l_l_hb = ( in & 0x00000000000000F0 ) >> 4;
    uint8_t l_h_lb = ( in & 0x0000000000000F00 ) >> 8;
    uint8_t l_h_hb = ( in & 0x000000000000F000 ) >> 12;
    uint8_t h_l_lb = ( in & 0x00000000000F0000 ) >> 16;
    uint8_t h_l_hb = ( in & 0x0000000000F00000 ) >> 20;
    uint8_t h_h_lb = ( in & 0x000000000F000000 ) >> 24;
    uint8_t h_h_hb = ( in & 0x00000000F0000000 ) >> 28;
    uint8_t hh_l_lb = ( in & 0x0000000F00000000 ) >> 32;
    uint8_t hh_l_hb = ( in & 0x000000F000000000 ) >> 36;
    uint8_t hh_h_lb = ( in & 0x00000F0000000000 ) >> 40;
    uint8_t hh_h_hb = ( in & 0x0000F00000000000 ) >> 44;
    uint8_t hhh_l_lb = ( in & 0x000F000000000000 ) >> 48;
    uint8_t hhh_l_hb = ( in & 0x00F0000000000000 ) >> 52;
    uint8_t hhh_h_lb = ( in & 0x0F00000000000000 ) >> 56;
    uint8_t hhh_h_hb = ( in & 0xF000000000000000 ) >> 60;
    bool b_zero = !b_leading_zeros;

    _impl_print_hex_header();
    _impl_print_hex( hhh_h_hb, &b_zero );
    _impl_print_hex( hhh_h_lb, &b_zero );
    _impl_print_hex( hhh_l_hb, &b_zero );
    _impl_print_hex( hhh_l_lb, &b_zero );
    _impl_print_hex( hh_h_hb, &b_zero );
    _impl_print_hex( hh_h_lb, &b_zero );
    _impl_print_hex( hh_l_hb, &b_zero );
    _impl_print_hex( hh_l_lb, &b_zero );
    _impl_print_hex( h_h_hb, &b_zero );
    _impl_print_hex( h_h_lb, &b_zero );
    _impl_print_hex( h_l_hb, &b_zero );
    _impl_print_hex( h_l_lb, &b_zero );
    _impl_print_hex( l_h_hb, &b_zero );
    _impl_print_hex( l_h_lb, &b_zero );
    _impl_print_hex( l_l_hb, &b_zero );
    _impl_print_hex( l_l_lb, &b_zero );

    if( b_zero ) { axk_terminal_printc( '0' ); }
}

void axk_terminal_dumpmem( void* offset, uint32_t count )
{
    axk_terminal_printnl();
    axk_terminal_prints( "---------------------------------------------------------\n" );
    axk_terminal_prints( "zOS Memory Dump from " );
    axk_terminal_printh64_lz( (uint64_t)(uintptr_t) offset, true );
    axk_terminal_prints( " : \n\n" );

    for( int i = 0; i < count; i++ )
    {
        void* pos = (uint8_t*)offset + i;
        axk_terminal_printh8_lz( *((uint8_t*)pos), true );
        axk_terminal_printc( ' ' );

        if( ( i + 1 ) != count && ( i + 1 ) % 8 == 0 ) { axk_terminal_printnl(); }
    }

    axk_terminal_printnl();
    axk_terminal_prints( "-----------------------------------------------------\n" );
}

void axk_terminal_printnl( void )
{
    // For printing a new line, we simply skip to the next line and check if the terminal is full
    if( ++terminal_y >= TERMINAL_HEIGHT )
    {
        axk_terminal_handle_full();
    }
    else
    {
        terminal_x = 0;
    }
}

void axk_terminal_printtab( void )
{
    // Tabs are simply going to be replaced with a set number of spaces
    for( uint32_t i = 0; i < TERMINAL_TAB_SIZE; i++ )
    {
        axk_terminal_printc( ' ' );
    }
}

void axk_terminal_set_color_overwrite( bool enabled )
{
    terminal_use_color = enabled;
}

void axk_terminal_setbg( enum TERMINAL_COLOR color )
{
    // Were not going to bother checking if the color is valid
    terminal_bg = (uint8_t)color;
}

void axk_terminal_setfg( enum TERMINAL_COLOR color )
{
    terminal_fg = (uint8_t) color;
}

void axk_terminal_fill_bg_color( enum TERMINAL_COLOR color )
{
    // Go through and set the color for each char in the terminal buffer
    uint32_t terminalSize = TERMINAL_WIDTH * TERMINAL_HEIGHT * 2;
    for( uint32_t i = 1; i < terminalSize; i += 2 )
    {
        uint8_t* ptr = (uint8_t*)ptr_terminal_buffer + i;
        
        // So, we want to modify 3 of the upper bits only
        uint8_t old_value = *ptr;
        *ptr = ( 0b00000111 & (uint8_t) color ) << 4 | ( 0b00001111 & old_value );
    }

    terminal_bg = (uint8_t) color;
}

void axk_terminal_fill_fg_color( enum TERMINAL_COLOR color )
{
    // Go through and set the color for each char in the terminal buffer
    uint32_t terminal_size = TERMINAL_WIDTH * TERMINAL_HEIGHT * 2;
    for( uint32_t i = 1; i < terminal_size; i += 2 )
    {
        uint8_t* ptr = (uint8_t*)ptr_terminal_buffer + i;

        // We need to modify the lower 4 bits
        uint8_t old_value = *ptr;
        *ptr = ( 0b01110000 & (uint8_t) old_value ) | ( 0b00001111 & (uint8_t) color );
    }

    terminal_fg = (uint8_t) color;
}

enum TERMINAL_COLOR axk_terminal_getbg( void )
{
    return (enum TERMINAL_COLOR) terminal_bg;
}

enum TERMINAL_COLOR axk_terminal_getfg( void )
{
    return (enum TERMINAL_COLOR) terminal_fg;
}

uint32_t axk_terminal_getrow( void )
{
    return terminal_y;
}

uint32_t axk_terminal_getcol( void )
{
    return terminal_x;
}

bool axk_terminal_is_color_overwrite_enabled( void )
{
    return terminal_use_color;
}
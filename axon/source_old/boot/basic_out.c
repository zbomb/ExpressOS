/*==============================================================
    Axon Kernel - Basic Output
    2021, Zachary Berry
    axon/boot/basic_out.c
==============================================================*/

#include "axon/boot/basic_out.h"
#include "axon/boot/boot_params.h"
#include "axon/library/spinlock.h"
#include <string.h>

/*
    Constants
*/
#define BASICOUT_BORDER_SIZE        8
#define BASICOUT_CHAR_EXTRA_WIDTH   1
#define BASICOUT_CHAR_EXTRA_HEIGHT  1
#define BASICOUT_FONT_HEIGHT        16
#define BASICOUT_FONT_WIDTH         8

/*
    State
*/
static const struct axk_bootparams_framebuffer_t* pframebuffer  = NULL;
static struct axk_psf1_t basic_font;

static uint32_t pos_x   = BASICOUT_BORDER_SIZE;
static uint32_t pos_y   = BASICOUT_BORDER_SIZE;

static uint8_t fg_r     = 255;
static uint8_t fg_g     = 255;
static uint8_t fg_b     = 255;
static uint8_t bg_r     = 0;
static uint8_t bg_g     = 0;
static uint8_t bg_b     = 0;

static struct axk_spinlock_t lock;

extern char _binary_data_fonts_basic_out_psf_start;
extern char _binary_data_fonts_basic_out_psf_end;

/*
    Functions
*/
bool axk_basicout_init()
{
    // Store framebuffer pointer
    pframebuffer = axk_bootparams_get_framebuffer();

    if( pframebuffer == NULL )
    {
        pframebuffer = NULL;
        return false;
    }

    // Load the font data from the kernel binary, starting with the header
    memcpy( &( basic_font.header ), &_binary_data_fonts_basic_out_psf_start, sizeof( struct axk_psf1_header_t ) );
    if( basic_font.header.magic != PSF1_FONT_MAGIC ) 
    {
        return false;
    }

    // Find the start of the data section
    basic_font.glyph_data = (uint8_t*)( &_binary_data_fonts_basic_out_psf_start + sizeof( struct axk_psf1_header_t ) );

    // Initialize spin lock
    axk_spinlock_init( &lock );

    return true;
}


void axk_basicout_test( void )
{
    // Lets try and write a simple character

}


void axk_basicout_lock( void )
{
    axk_spinlock_acquire( &lock );
}


void axk_basicout_unlock( void )
{
    axk_spinlock_release( &lock );
}


void axk_basicout_set_fg( uint8_t r, uint8_t g, uint8_t b )
{
    fg_r = r; fg_g = g; fg_b = b;
}


void axk_basicout_set_bg( uint8_t r, uint8_t g, uint8_t b )
{
    bg_r = r; bg_g = g; bg_b = b;
}


void _print_word( const char* str, size_t count )
{
    // First, determine the size of the word and if it can fit on the current line
    if( str == NULL || count == 0UL ) { return; }
    uint32_t word_sz = ( count * ( 8 + BASICOUT_CHAR_EXTRA_WIDTH ) );

    uint32_t screen_w = pframebuffer->resolution.width;
    uint32_t screen_h = pframebuffer->resolution.height;

    // So, were going to push the word to the next line if it cant fit on the current line
    // There is an exception to this rule though: If the word is very long (i.e. larger than or equal to 1/4 of the screen width) we will
    // go to the next line once the next character wont fit on the current line
    bool b_split_word = ( word_sz >= ( screen_w - BASICOUT_BORDER_SIZE - BASICOUT_BORDER_SIZE ) / 4U );
    if( !b_split_word && ( pos_x + word_sz > ( screen_w - BASICOUT_BORDER_SIZE ) ) )
    {
        axk_basicout_printnl();
    }

    // Now, the position should be accurate, and we should have enough room to print
    uint32_t glyph_count = basic_font.header.mode == 1 ? 512 : 256;
    for( size_t index = 0; index < count; index++ )
    {
        if( b_split_word && ( pos_x + 8 + BASICOUT_CHAR_EXTRA_WIDTH ) > ( screen_w - BASICOUT_BORDER_SIZE ) )
        {
            axk_basicout_printnl();
        }

        char c = str[ index ];
        if( c >= glyph_count )
        {
            c = (char)( 0 );
        }

        // Now, loop through the pixels affected by this letter being printed (including any padding), and determine if its foreground or background
        uint8_t* gdata = (uint8_t*)( basic_font.glyph_data ) + ( basic_font.header.glyph_sz * (uint32_t)( c ) );
        uint8_t* fline = (uint8_t*)( pframebuffer->buffer ) + ( pframebuffer->resolution.pixels_per_scanline * 4U * pos_y );

        for( uint32_t y = 0; y < 16; y++ )
        {
            for( uint32_t x = 0; x < 8; x++ )
            {
                uint32_t xoff   = ( x + pos_x ) * 4U;
                bool b_fg       = ( ( *gdata & ( 0b10000000 >> x ) ) != 0 );

                switch( pframebuffer->resolution.format )
                {
                    case PIXEL_FORMAT_BGRX_32:
                    {
                        fline[ xoff + 0U ] = b_fg ? fg_b : bg_b;
                        fline[ xoff + 1U ] = b_fg ? fg_g : bg_g;
                        fline[ xoff + 2U ] = b_fg ? fg_r : bg_r;

                        break;
                    }

                    case PIXEL_FORMAT_RGBX_32:
                    {
                        fline[ xoff + 0U ] = b_fg ? fg_r : bg_r;
                        fline[ xoff + 1U ] = b_fg ? fg_g : bg_g;
                        fline[ xoff + 2U ] = b_fg ? fg_b : bg_b;

                        break;
                    }
                }
            }

            gdata++;
            fline += ( pframebuffer->resolution.pixels_per_scanline * 4U );
        }

        pos_x += 8 + BASICOUT_CHAR_EXTRA_WIDTH;

    }
}

void axk_basicout_prints( const char* str )
{
    if( str == NULL ) { return; }
    
    size_t start_word   = 0UL;
    size_t index        = 0UL;

    while( true )
    {
        char c = *( str + index );
        bool is_space = ( c == ' ' );
        bool is_end = ( c == 0x00 );
        bool is_tab = ( c == '\t' );
        bool is_nl = ( c == '\n' );

        if( is_end || is_space || is_tab || is_nl )
        {
            // We found the end of a word or the end of the string
            if( start_word < index )
            {
                // More than one valid character in the word, so lets print it
                _print_word( str + start_word, index - start_word + ( is_space ? 1UL : 0UL ) );
            }

            if( is_end ) { break; }
            if( is_tab ) { _print_word( "    ", 4 ); }
            if( is_nl ) { axk_basicout_printnl(); }

            // The next word is assumed to start on the next character after the current space character
            start_word = index + 1UL;
        }

        index++;
    }
}


void axk_basicout_printnl( void )
{
    uint32_t screen_w = pframebuffer->resolution.width;
    uint32_t screen_h = pframebuffer->resolution.height;

    // Go to a new line
    pos_x = BASICOUT_BORDER_SIZE;
    
    if( ( pos_y + ( 16 + BASICOUT_CHAR_EXTRA_HEIGHT ) * 2U ) > ( screen_h - BASICOUT_BORDER_SIZE ) )
    {
        // We need to take all existing text and scroll up a line
        // So, were going to copy all screen data 
        uint32_t line_pitch     = pframebuffer->resolution.pixels_per_scanline * 4UL;
        uint32_t needed_lines   = ( ( ( 16 + BASICOUT_CHAR_EXTRA_HEIGHT ) * 2U ) + pos_y ) - ( screen_h - BASICOUT_BORDER_SIZE );
        uint32_t source_y       = ( BASICOUT_BORDER_SIZE + needed_lines );
        uint32_t dest_y         = BASICOUT_BORDER_SIZE;
        uint32_t copy_count     = ( 16 - ( BASICOUT_BORDER_SIZE * 2 ) ) - needed_lines;

        memmove( (void*)( (uint8_t*)( pframebuffer->buffer ) + ( dest_y * line_pitch ) ),
            (void*)( (uint8_t*)( pframebuffer->buffer ) + ( source_y * line_pitch ) ),
            copy_count * line_pitch );
        
        // Now, we can adjust the current Y position to be at the bottom of the shifted text
        pos_y = ( pos_y + 16 + BASICOUT_CHAR_EXTRA_HEIGHT ) - needed_lines;
    }
    else
    {
        pos_y += ( 16 + BASICOUT_CHAR_EXTRA_HEIGHT );
    }
}


void axk_basicout_printtab( void )
{
    axk_basicout_prints( "\t" );
}


void axk_basicout_printu32( uint32_t num )
{
    if( num == 0U )
    {
        axk_basicout_prints( "0" );
    }
    else
    {
        // Loop through each digit, get its value and print the corresponding character
        // Max value of a uint32_t has 10 digits in base 10
        uint32_t place_value = 1000000000;
        bool b_zero = true;
        char str_data[ 11 ];
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

            str_data[ index++ ] = (char)( digit + 0x30 );

            num -= digit * place_value;
            place_value /= 10;
        }
        
        // Write null terminator character at the end of the string data
        str_data[ index ] = 0x00;
        axk_basicout_prints( str_data );
    }
}


void axk_basicout_printu64( uint64_t num )
{
    if( num == 0UL )
    {
        axk_basicout_prints( "0" );
    }
    else if( num <= 0xFFFFFFFF )
    {
        axk_basicout_printu32( (uint32_t) num );
    }
    else
    {
        uint64_t place_value = 10000000000000000000UL;
        bool b_zero = true;
        char str_data[ 21 ];
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

            str_data[ index++ ] = (char)( digit + 0x30 );

            num -= digit * place_value;
            place_value /=  10UL;
        }

        // Write null terminator and print string
        str_data[ index ] = 0x00;
        axk_basicout_prints( str_data );
    }
}

static uint8_t _print_hex_byte( uint8_t in_byte, char* out_chars, bool* b_print_lz )
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


void axk_basicout_printh32( uint32_t num, bool b_leading_zeros )
{
    char str_data[ 11 ];
    uint8_t index = 0;

    // Write the hex header
    str_data[ index++ ] = '0';
    str_data[ index++ ] = 'x';

    // Write each byte in hex
    index += _print_hex_byte( ( num & 0xFF000000 ) >> 24, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x00FF0000 ) >> 16, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x0000FF00 ) >> 8, str_data + index, &b_leading_zeros );
    index += _print_hex_byte( ( num & 0x000000FF ), str_data + index, &b_leading_zeros );

    // If we trimmed leading zeros, and the string was empty, we need to at laest print one zero digit
    if( !b_leading_zeros )
    {
        str_data[ index++ ] = '0';
    }

    // Write null terminator
    str_data[ index ] = 0x00;

    // Print the string
    axk_basicout_prints( str_data );
}


void axk_basicout_printh64( uint64_t num, bool b_leading_zeros )
{
    char str_data[ 19 ];
    uint8_t index = 0;

    // Write the hex header
    str_data[ index++ ] = '0';
    str_data[ index++ ] = 'x';

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
        str_data[ index++ ] = '0';
    }

    // Write null terminator
    str_data[ index ] = 0x00;

    // Print the string
    axk_basicout_prints( str_data );
}


void axk_basicout_clear( void )
{
    pos_x = BASICOUT_BORDER_SIZE;
    pos_y = BASICOUT_BORDER_SIZE;

    // Clear the framebuffer
    memset( pframebuffer->buffer, 0, pframebuffer->size );
}


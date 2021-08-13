/*==============================================================
    Axon Kernel - Basic Terminal Library
    2021, Zachary Berry
    axon/gfx/basic_terminal.c
==============================================================*/

#include "axon/gfx/basic_terminal.h"
#include "axon/library/spinlock.h"

/*
    Constants
*/
#define BASIC_TERMINAL_BORDER_SIZE          8
#define BASIC_TERMINAL_CHAR_EXTRA_WIDTH     1
#define BASIC_TERMINAL_CHAR_EXTRA_HEIGHT    1
#define BASIC_TERMINAL_FONT_HEIGHT          16
#define BASIC_TERMINAL_FONT_WIDTH           8

/*
    State
*/
static struct tzero_framebuffer_t g_framebuffer;
static struct axk_psf1_t g_font;

static uint32_t g_pos_x     = BASIC_TERMINAL_BORDER_SIZE;
static uint32_t g_pos_y     = BASIC_TERMINAL_BORDER_SIZE;
static uint8_t g_fg_r       = 255;
static uint8_t g_fg_g       = 255;
static uint8_t g_fg_b       = 255;
static uint8_t g_bg_r       = 0;
static uint8_t g_bg_g       = 0;
static uint8_t g_bg_b       = 0;

static enum axk_basicterminal_mode_t g_mode = BASIC_TERMINAL_MODE_CONSOLE;
static struct axk_spinlock_t g_lock;

extern char _binary_data_fonts_basic_terminal_psf_start;
extern char _binary_data_fonts_basic_terminal_psf_end;

/*
    Functions
*/
bool axk_basicterminal_init( struct tzero_payload_parameters_t* in_params )
{
    // Copy in the framebuffer information
    if( in_params == NULL ) { return false; }   
    memcpy( &g_framebuffer, &( in_params->framebuffer ), sizeof( struct tzero_framebuffer_t ) );

    // Load the font data from the kernel binary, starting with the header
    memcpy( &( g_font.header ), &_binary_data_fonts_basic_terminal_psf_start, sizeof( struct axk_psf1_header_t ) );
    if( g_font.header.magic != PSF1_FONT_MAGIC ) 
    {
        return false;
    }

    // Find the start of the data section
    g_font.glyph_data   = (uint8_t*)( &_binary_data_fonts_basic_terminal_psf_start + sizeof( struct axk_psf1_header_t ) );
    g_mode              = BASIC_TERMINAL_MODE_CONSOLE;

    // Initialize spin lock
    axk_spinlock_init( &g_lock );

    return true;
}


enum axk_basicterminal_mode_t axk_basicterminal_get_mode( void )
{
    return g_mode;
}


void axk_basicterminal_set_mode( enum axk_basicterminal_mode_t in_mode )
{
    if( in_mode != g_mode )
    {
        g_mode = in_mode;
        axk_basicterminal_clear();
    }
}


void axk_basicterminal_lock( void )
{
    axk_spinlock_acquire( &g_lock );
}


void axk_basicterminal_unlock( void )
{
    axk_spinlock_release( &g_lock );
}


void axk_basicterminal_set_fg( uint8_t r, uint8_t g, uint8_t b )
{
    g_fg_r = r; g_fg_g = g; g_fg_b = b;
}


void axk_basicterminal_set_bg( uint8_t r, uint8_t g, uint8_t b )
{
    g_bg_r = r; g_bg_g = g; g_bg_b = b;
}


void axk_basicterminal_get_size( uint32_t* out_width, uint32_t* out_height )
{
    if( out_width != NULL )     { *out_width = g_framebuffer.resolution.width; }
    if( out_height != NULL )    { *out_height = g_framebuffer.resolution.height; }
}


void axk_basicterminal_get_text_size( const char* str, uint32_t* out_width, uint32_t* out_height )
{
    uint32_t width      = 0U;
    uint32_t height     = 0U;

    if( str != NULL )
    {
        size_t len      = strlen( str );
        size_t full_w   = len * ( BASIC_TERMINAL_FONT_WIDTH + BASIC_TERMINAL_CHAR_EXTRA_WIDTH );
        size_t full_h   = BASIC_TERMINAL_FONT_HEIGHT + BASIC_TERMINAL_CHAR_EXTRA_HEIGHT;

        width   = (uint32_t)( full_w > 0xFFFFFFFFUL ? 0xFFFFFFFFUL : full_w );
        height  = (uint32_t)( full_h > 0xFFFFFFFFUL ? 0xFFFFFFFFUL : full_h );
    }

    if( out_width != NULL )     { *out_width = width; }
    if( out_height != NULL )    { *out_height = height; }
}


void _print_word( const char* str, size_t count )
{
    // First, determine the size of the word and if it can fit on the current line
    if( str == NULL || count == 0UL ) { return; }
    uint32_t word_sz = ( count * ( 8 + BASIC_TERMINAL_CHAR_EXTRA_WIDTH ) );

    uint32_t screen_w = g_framebuffer.resolution.width;
    uint32_t screen_h = g_framebuffer.resolution.height;

    // So, were going to push the word to the next line if it cant fit on the current line
    // There is an exception to this rule though: If the word is very long (i.e. larger than or equal to 1/4 of the screen width) we will
    // go to the next line once the next character wont fit on the current line
    bool b_split_word = ( word_sz >= ( screen_w - BASIC_TERMINAL_BORDER_SIZE - BASIC_TERMINAL_BORDER_SIZE ) / 4U );
    if( !b_split_word && ( g_pos_x + word_sz > ( screen_w - BASIC_TERMINAL_BORDER_SIZE ) ) )
    {
        axk_basicterminal_printnl();
    }

    // Now, the position should be accurate, and we should have enough room to print
    uint32_t glyph_count = g_font.header.mode == 1 ? 512 : 256;
    for( size_t index = 0; index < count; index++ )
    {
        if( b_split_word && ( g_pos_x + 8 + BASIC_TERMINAL_CHAR_EXTRA_WIDTH ) > ( screen_w - BASIC_TERMINAL_BORDER_SIZE ) )
        {
            axk_basicterminal_printnl();
        }

        char c = str[ index ];
        if( c >= glyph_count )
        {
            c = (char)( 0 );
        }

        // Now, loop through the pixels affected by this letter being printed (including any padding), and determine if its foreground or background
        uint8_t* gdata = (uint8_t*)( g_font.glyph_data ) + ( g_font.header.glyph_sz * (uint32_t)( c ) );
        uint8_t* fline = (uint8_t*)( g_framebuffer.phys_addr ) + ( g_framebuffer.resolution.pixels_per_scanline * 4U * g_pos_y );

        for( uint32_t y = 0; y < 16; y++ )
        {
            for( uint32_t x = 0; x < 8; x++ )
            {
                uint32_t xoff   = ( x + g_pos_x ) * 4U;
                bool b_fg       = ( ( *gdata & ( 0b10000000 >> x ) ) != 0 );

                // Check for common modes we can have optimized algorithms for better rendering performance
                if( g_framebuffer.resolution.mode == PIXEL_FORMAT_RGBX_32 )
                {
                    fline[ xoff + 0U ] = b_fg ? g_fg_r : g_bg_r;
                    fline[ xoff + 1U ] = b_fg ? g_fg_g : g_bg_g;
                    fline[ xoff + 2U ] = b_fg ? g_fg_b : g_bg_b;
                }
                else if( g_framebuffer.resolution.mode == PIXEL_FORMAT_BGRX_32 )
                {
                    fline[ xoff + 0U ] = b_fg ? g_fg_b : g_bg_b;
                    fline[ xoff + 1U ] = b_fg ? g_fg_g : g_bg_g;
                    fline[ xoff + 2U ] = b_fg ? g_fg_r : g_bg_r;
                }
                else
                {
                    // First, lets do the red channel
                    uint32_t pixel      = 0x00000000U;
                    uint8_t r_bit_diff  = 8 - g_framebuffer.resolution.red_bit_width;

                    if( r_bit_diff == 0 )
                    {
                        pixel |= ( ( b_fg ? g_fg_r : g_bg_r ) << g_framebuffer.resolution.red_shift );
                    }
                    else
                    {
                        uint8_t divisor = 1;
                        for( uint8_t i = 0; i < r_bit_diff; i++ ) { divisor *= 2; }
                        pixel |= ( (uint32_t)( ( b_fg ? g_fg_r : g_bg_r ) / divisor ) << g_framebuffer.resolution.red_shift );
                    }

                    // Green channel
                    uint8_t g_bit_diff = 8 - g_framebuffer.resolution.green_bit_width;
                    if( g_bit_diff == 0 )
                    {
                        pixel |= ( ( b_fg ? g_fg_g : g_bg_g ) << g_framebuffer.resolution.green_shift );
                    }
                    else
                    {
                        uint8_t divisor = 1;
                        for( uint8_t i = 0; i < g_bit_diff; i++ ) { divisor *= 2; }
                        pixel |= ( (uint32_t)( ( b_fg ? g_fg_g : g_bg_g ) / divisor ) << g_framebuffer.resolution.green_shift );
                    }

                    // Blue channel
                    uint8_t b_bit_diff = 8 - g_framebuffer.resolution.blue_bit_width;
                    if( b_bit_diff == 0 )
                    {
                        pixel |= ( ( b_fg ? g_fg_b : g_bg_b ) << g_framebuffer.resolution.blue_shift );
                    }
                    else
                    {
                        uint8_t divisor = 1;
                        for( uint8_t i = 0; i < b_bit_diff; i++ ) { divisor *= 2; }
                        pixel |= ( (uint32_t)( ( b_fg ? g_fg_b : g_bg_b ) / divisor ) << g_framebuffer.resolution.blue_shift );
                    }

                    // Write the pixel 
                    *( (uint32_t*)( fline + xoff ) ) = pixel;
                }
            }

            gdata++;
            fline += ( g_framebuffer.resolution.pixels_per_scanline * 4U );
        }

        g_pos_x += 8 + BASIC_TERMINAL_CHAR_EXTRA_WIDTH;

    }
}

void axk_basicterminal_prints( const char* str )
{
    if( str == NULL || g_mode != BASIC_TERMINAL_MODE_CONSOLE ) { return; }
    
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
            if( is_nl ) { axk_basicterminal_printnl(); }

            // The next word is assumed to start on the next character after the current space character
            start_word = index + 1UL;
        }

        index++;
    }
}


void axk_basicterminal_printnl( void )
{
    if( g_mode != BASIC_TERMINAL_MODE_CONSOLE ) { return; }

    uint32_t screen_w = g_framebuffer.resolution.width;
    uint32_t screen_h = g_framebuffer.resolution.height;

    // Go to a new line
    g_pos_x = BASIC_TERMINAL_BORDER_SIZE;
    
    if( ( g_pos_y + ( 16 + BASIC_TERMINAL_CHAR_EXTRA_HEIGHT ) * 2U ) > ( screen_h - BASIC_TERMINAL_BORDER_SIZE ) )
    {
        // We need to take all existing text and scroll up a line
        // So, were going to copy all screen data 
        uint32_t line_pitch     = g_framebuffer.resolution.pixels_per_scanline * 4UL;
        uint32_t needed_lines   = ( ( ( 16 + BASIC_TERMINAL_CHAR_EXTRA_HEIGHT ) * 2U ) + g_pos_y ) - ( screen_h - BASIC_TERMINAL_BORDER_SIZE );
        uint32_t source_y       = ( BASIC_TERMINAL_BORDER_SIZE + needed_lines );
        uint32_t dest_y         = BASIC_TERMINAL_BORDER_SIZE;
        uint32_t copy_count     = ( 16 - ( BASIC_TERMINAL_BORDER_SIZE * 2 ) ) - needed_lines;

        memmove( (void*)( (uint8_t*)( g_framebuffer.phys_addr ) + ( dest_y * line_pitch ) ),
            (void*)( (uint8_t*)( g_framebuffer.phys_addr ) + ( source_y * line_pitch ) ),
            copy_count * line_pitch );
        
        // Now, we can adjust the current Y position to be at the bottom of the shifted text
        g_pos_y = ( g_pos_y + 16 + BASIC_TERMINAL_CHAR_EXTRA_HEIGHT ) - needed_lines;
    }
    else
    {
        g_pos_y += ( 16 + BASIC_TERMINAL_CHAR_EXTRA_HEIGHT );
    }
}
 

void axk_basicterminal_printtab( void )
{
    if( g_mode != BASIC_TERMINAL_MODE_CONSOLE ) { return; }

    axk_basicterminal_prints( "\t" );
}


void axk_basicterminal_printu32( uint32_t num )
{
    if( g_mode != BASIC_TERMINAL_MODE_CONSOLE ) { return; }

    if( num == 0U )
    {
        axk_basicterminal_prints( "0" );
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
        axk_basicterminal_prints( str_data );
    }
}


void axk_basicterminal_printu64( uint64_t num )
{
    if( g_mode != BASIC_TERMINAL_MODE_CONSOLE ) { return; }

    if( num == 0UL )
    {
        axk_basicterminal_prints( "0" );
    }
    else if( num <= 0xFFFFFFFF )
    {
        axk_basicterminal_printu32( (uint32_t) num );
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
        axk_basicterminal_prints( str_data );
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


void axk_basicterminal_printh32( uint32_t num, bool b_leading_zeros )
{
    if( g_mode != BASIC_TERMINAL_MODE_CONSOLE ) { return; }

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
    axk_basicterminal_prints( str_data );
}


void axk_basicterminal_printh64( uint64_t num, bool b_leading_zeros )
{
    if( g_mode != BASIC_TERMINAL_MODE_CONSOLE ) { return; }

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
    axk_basicterminal_prints( str_data );
}


void axk_basicterminal_clear( void )
{
    g_pos_x = BASIC_TERMINAL_BORDER_SIZE;
    g_pos_y = BASIC_TERMINAL_BORDER_SIZE;

    // Clear the framebuffer
    memset( (void*)g_framebuffer.phys_addr, 0, g_framebuffer.size );
}


static inline void _draw_pixel( uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b )
{
    if( x >= g_framebuffer.resolution.width || y >= g_framebuffer.resolution.height ) { return; }

    // Find address of this pixel in the framebuffer
    uint8_t* pixel = (uint8_t*)( g_framebuffer.phys_addr + ( g_framebuffer.resolution.pixels_per_scanline * 4U * y ) + ( x * 4U ) );
    switch( g_framebuffer.resolution.mode )
    {
        case PIXEL_FORMAT_RGBX_32:
        {
            pixel[ 0 ] = r;
            pixel[ 1 ] = g;
            pixel[ 2 ] = b;
            break;
        }

        case PIXEL_FORMAT_BGRX_32:
        {
            pixel[ 0 ] = b;
            pixel[ 1 ] = g;
            pixel[ 2 ] = r;
            break;
        }

        default:
        {
            // First, lets do the red channel
            uint32_t data      = 0x00000000U;
            uint8_t r_bit_diff  = 8 - g_framebuffer.resolution.red_bit_width;

            if( r_bit_diff == 0 )
            {
                data |= ( (uint32_t)( r ) << g_framebuffer.resolution.red_shift );
            }
            else
            {
                uint8_t divisor = 1;
                for( uint8_t i = 0; i < r_bit_diff; i++ ) { divisor *= 2; }
                data |= ( (uint32_t)( r / divisor ) << g_framebuffer.resolution.red_shift );
            }

            // Green channel
            uint8_t g_bit_diff = 8 - g_framebuffer.resolution.green_bit_width;
            if( g_bit_diff == 0 )
            {
                data |= ( (uint32_t)( g ) << g_framebuffer.resolution.green_shift );
            }
            else
            {
                uint8_t divisor = 1;
                for( uint8_t i = 0; i < g_bit_diff; i++ ) { divisor *= 2; }
                data |= ( (uint32_t)( g / divisor ) << g_framebuffer.resolution.green_shift );
            }

            // Blue channel
            uint8_t b_bit_diff = 8 - g_framebuffer.resolution.blue_bit_width;
            if( b_bit_diff == 0 )
            {
                data |= ( (uint32_t)( b ) << g_framebuffer.resolution.blue_shift );
            }
            else
            {
                uint8_t divisor = 1;
                for( uint8_t i = 0; i < b_bit_diff; i++ ) { divisor *= 2; }
                data |= ( (uint32_t)( b / divisor ) << g_framebuffer.resolution.blue_shift );
            }

            // Write the pixel 
            *( (uint32_t*)( pixel ) ) = data;
            break;
        }
    }
}


void axk_basicterminal_draw_text( const char* str, uint32_t in_x, uint32_t in_y, bool b_transparent_bg )
{
    if( g_mode != BASIC_TERMINAL_MODE_GRAPHICS || str == NULL || in_x >= g_framebuffer.resolution.width || in_y >= g_framebuffer.resolution.height ) { return; }

    size_t str_sz = strlen( str );

    for( size_t i = 0; i < str_sz; i++ )
    {
        char c = str[ i ];

        // Now, loop through the pixels affected by this letter being printed (including any padding), and determine if its foreground or background
        uint8_t* gdata = (uint8_t*)( g_font.glyph_data ) + ( g_font.header.glyph_sz * (uint32_t)( c ) );
        for( uint32_t y = 0; y < 16; y++ )
        {
            for( uint32_t x = 0; x < 8; x++ )
            {
                bool b_fg = ( ( *gdata & ( 0b10000000 >> x ) ) != 0 );
                if( !b_fg && b_transparent_bg ) { continue; }

                _draw_pixel( x + in_x, y + in_y, b_fg ? g_fg_r : g_bg_r, b_fg ? g_fg_g : g_bg_g, b_fg ? g_fg_b : g_bg_g );
            }

            gdata++;
        }

        in_x += 8 + BASIC_TERMINAL_CHAR_EXTRA_WIDTH;
    }
}


void axk_basicterminal_draw_text_box( const char* str, uint32_t in_x, uint32_t in_y, uint32_t in_w, uint32_t in_h, bool b_transparent_bg )
{
    if( g_mode != BASIC_TERMINAL_MODE_GRAPHICS || in_x >= g_framebuffer.resolution.width || in_y >= g_framebuffer.resolution.height ) { return; }

    uint32_t max_x = in_x + in_w;
    uint32_t max_y = in_y + in_h;

    // First, draw the background if desired
    if( !b_transparent_bg )
    {
        for( uint32_t y = in_y; y < max_y; y++ )
        {
            for( uint32_t x = in_x; x < max_x; x++ )
            {
                _draw_pixel( x, y, g_bg_r, g_bg_g, g_bg_b );
            }
        }
    }

    // Next, we want to loop through each character in the string, drawing each inside of the text box
    // If we dont have enough room left on the line, we go to the next line
    // Ensure we dont overdraw past the end of the box, we can draw partial characters though
    // Once the current line position is totally past the bottom of the text box, we will break
    if( str == NULL ) { return; }
    size_t str_len = strlen( str );
    if( str_len == 0UL ) { return; }

    uint32_t line = in_y;
    uint32_t xpos = in_x;
    size_t str_index = 0UL;

    while( line < max_y && str_index < str_len )
    {
        // Check if the next character will fit, otherwise go to the next line
        if( xpos + BASIC_TERMINAL_FONT_WIDTH + BASIC_TERMINAL_CHAR_EXTRA_WIDTH > max_x )
        {
            line += ( BASIC_TERMINAL_FONT_HEIGHT + BASIC_TERMINAL_CHAR_EXTRA_HEIGHT );
            xpos = in_x;
            continue;
        }

        // Draw the next character
        // TODO: We should instead draw word-by-word, unless the word is very large, then we go character by character
        char c = str[ str_index++ ];
        uint8_t* char_data = (uint8_t*)( g_font.glyph_data ) + ( g_font.header.glyph_sz * (uint32_t)( c ) );
        for( uint32_t y = 0; y < BASIC_TERMINAL_FONT_HEIGHT; y++ )
        {
            if( ( line + y ) >= max_y ) { break; }
            for( uint32_t x = 0; x < BASIC_TERMINAL_FONT_WIDTH; x++ )
            {
                if( ( *char_data & ( 0b10000000 >> x ) ) != 0 )
                {
                    _draw_pixel( xpos + x, line + y, g_fg_r, g_fg_g, g_fg_b );
                }
            }

            char_data++;
        }

        // Increment x-position
        xpos += ( BASIC_TERMINAL_FONT_WIDTH + BASIC_TERMINAL_CHAR_EXTRA_WIDTH );
    }
}


void axk_basicterminal_draw_number( uint64_t num, uint32_t x, uint32_t y, bool b_transparent_bg )
{
    if( g_mode != BASIC_TERMINAL_MODE_GRAPHICS || x >= g_framebuffer.resolution.width || y >= g_framebuffer.resolution.height ) { return; }

    if( num == 0UL )
    {
        axk_basicterminal_draw_text( "0", x, y, b_transparent_bg );
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
        axk_basicterminal_draw_text( str_data, x, y, b_transparent_bg );
    }
}


void axk_basicterminal_draw_hex( uint64_t num, bool b_leading_zeros, uint32_t x, uint32_t y, bool b_transparent_bg )
{
    if( g_mode != BASIC_TERMINAL_MODE_GRAPHICS || x >= g_framebuffer.resolution.width || y >= g_framebuffer.resolution.height ) { return; }

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
    axk_basicterminal_draw_text( str_data, x, y, b_transparent_bg );
}


void axk_basicterminal_draw_pixel( uint32_t x, uint32_t y )
{
    _draw_pixel( x, y, g_fg_r, g_fg_g, g_fg_b );
}


void axk_basicterminal_draw_box( uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t outline_width )
{
    if( g_mode != BASIC_TERMINAL_MODE_GRAPHICS || x >= g_framebuffer.resolution.width || y >= g_framebuffer.resolution.height ) { return; }

    // To draw an outlined box, lets start by filling the rectangle with foreground color
    uint32_t end_y = ( (uint64_t)( y ) + (uint64_t)( h ) >= (uint64_t)( g_framebuffer.resolution.height ) ? g_framebuffer.resolution.height : y + h );
    uint32_t end_x = ( (uint64_t)( x ) + (uint64_t)( w ) >= (uint64_t)( g_framebuffer.resolution.width ) ? g_framebuffer.resolution.width : x + w );
    uint32_t fixed_w = end_x - x;

    for( uint32_t py = y; py < end_y; py++ )
    {
        // Check for horizontal outlines
        bool b_horizontal_outline = ( ( py < ( y + outline_width ) ) || ( py >= ( end_y - outline_width ) ) );

        for( uint32_t px = 0; px < fixed_w; px++ )
        {
            // Check for vertical outlines
            bool b_outline = ( b_horizontal_outline || ( px < outline_width ) || ( px >= ( w - outline_width ) ) );
            _draw_pixel( px + x, py, b_outline ? g_bg_r : g_fg_r, b_outline ? g_bg_g : g_fg_g, b_outline ? g_bg_b : g_fg_b );
        }
    }
}   
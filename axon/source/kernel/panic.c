/*==============================================================
    Axon Kernel - Panic Function
    2021, Zachary Berry
    axon/source/kernel/panic.c
==============================================================*/

#include "axon/kernel/panic.h"
#include "axon/kernel/kernel.h"
#include "axon/gfx/basic_terminal.h"
#include "axon/library/spinlock.h"

/*
    State
*/
static struct axk_spinlock_t g_lock;
static bool b_panicing;
static uint32_t message_x;
static uint32_t message_y;
static uint32_t message_w;
static uint32_t message_h;
static uint32_t print_x;
static uint32_t print_y;
static void* ptr_pstate;


/*
    axk_panic_helper
    * Arch-specific function
    * Each architecture must implement this, it should stop the execution of all other processors on the system
*/
void axk_panic_helper( void );

/*
    Function Implementations
*/
void axk_panic_init( void )
{
    axk_spinlock_init( &g_lock );

    b_panicing  = false;
    message_x   = 0U;
    message_y   = 0U;
    message_w   = 0U;
    message_h   = 0U;
    print_x     = 0U;
    print_y     = 0U;
    ptr_pstate  = NULL;
}


__attribute__((noreturn, noinline)) void axk_panic( const char* str )
{
    axk_panic_begin();
    axk_panic_prints( str );
    axk_panic_end();
}


/*
    Advanced Panic Interface Functions
*/
__attribute__((noinline)) void axk_panic_begin( void )
{
    // Firstly, disable interrupts, we dont want the kernel to preempt and continue running code after a panic
    // We also need to gain a lock to ensure no other processors can overwrite this panic
    axk_spinlock_acquire( &g_lock );
    axk_basicterminal_lock();
    b_panicing = true;

    // Read the address this was called from
    uint64_t ret_addr = (uint64_t) __builtin_extract_return_addr( __builtin_return_address( 0 ) );

    // Call arch-specific function to stop all other processors
    //axk_panic_helper();

    // Switch the terminal over to graphics mode
    axk_basicterminal_set_mode( BASIC_TERMINAL_MODE_GRAPHICS );

    // Draw the background
    uint32_t screen_w, screen_h;
    axk_basicterminal_get_size( &screen_w, &screen_h );

    axk_basicterminal_set_fg( 200, 10, 10 );
    axk_basicterminal_set_bg( 20, 20, 20 );
    axk_basicterminal_draw_box( 0, 0, screen_w, screen_h, 8 );

    // Draw the header, and box surrounding it
    const char* str_header = "Axon Kernel Panic!";
    uint32_t padding = screen_h / 25U;
    uint32_t header_w, header_h;
    axk_basicterminal_get_text_size( str_header, &header_w, &header_h );

    axk_basicterminal_set_bg( 230, 230, 230 );
    axk_basicterminal_draw_box( ( screen_w / 2 ) - ( header_w / 2 ) - padding, padding, header_w + padding + padding, header_h + padding, 6 );
    axk_basicterminal_set_fg( 230, 230, 230 );
    axk_basicterminal_draw_text( str_header, ( screen_w / 2 ) - ( header_w / 2 ), padding + ( padding / 2 ), true );

    // Draw the first text box
    const char* str_panic_desc      = "The kernel has encountered an error that was unrecoverable, and was forced to halt the system. Please document and report this crash to the developers so we can continue to improve the kernel";
    axk_basicterminal_draw_text_box( str_panic_desc, padding + padding, header_h + ( padding * 3U ), screen_w - ( padding * 4U ), screen_h / 10U, true );

    // Draw info about where the error was triggered from and the error message
    const char* str_source = "The error was triggered from: ";
    uint32_t str_source_w, str_source_h;
    axk_basicterminal_get_text_size( str_source, &str_source_w, &str_source_h );
    uint32_t hex64_w, hex64_h;
    axk_basicterminal_get_text_size( "0x1234567812345678", &hex64_w, &hex64_h );
    axk_basicterminal_draw_text( str_source, ( screen_w / 2U ) - ( ( str_source_w + hex64_w ) / 2U ), ( screen_h ) / 4U, true );
    axk_basicterminal_draw_hex( ret_addr, true, ( screen_w / 2U ) - ( ( str_source_w + hex64_w ) / 2U ) + str_source_w, ( screen_h ) / 4U, true );

    // TODO: Eventually, it would be nice if we could give out a disassembly of the line of code, or maybe even the code file name and line number
    // but, this could end up being quite complex to implement, or greatly increase the size of the compiled kernel 

    // Draw 'error string header'
    uint32_t err_header_y = ( ( screen_h ) / 4U ) + ( str_source_h * 3U );
    axk_basicterminal_draw_box( padding, err_header_y, screen_w - ( padding * 2U ), 2, 0 );
    const char* str_err_header = "The error message provided was:";
    uint32_t err_header_w, err_header_h;
    axk_basicterminal_get_text_size( str_err_header, &err_header_w, &err_header_h );
    axk_basicterminal_draw_text( str_err_header, ( screen_w / 2 ) - ( err_header_w / 2 ), err_header_y + str_source_h, true );


    axk_basicterminal_draw_box( padding, ( screen_h * 7U ) / 11U, screen_w - ( padding * 2U ), 2, false );

    // Store the dimensions of where the error can be written to
    //axk_basicterminal_draw_text_box( str, padding + padding, err_header_y + ( str_source_h * 3U ), screen_w - ( padding * 4U ), screen_h / 4U, true );
    message_x   = padding + padding;
    message_y   = err_header_y + ( str_source_h * 3U );
    message_w   = screen_w - ( padding * 4U );
    message_h   = screen_h / 4U;
    print_x     = 0U;
    print_y     = 0U;

    // Draw system state header
    const char* str_state_header = "System State";
    uint32_t state_w, state_h;
    uint32_t state_y = ( ( screen_h * 7U ) / 11U ) + ( str_source_h );
    axk_basicterminal_get_text_size( str_state_header, &state_w, &state_h );
    axk_basicterminal_draw_text( str_state_header, ( screen_w / 2U ) - ( state_w / 2U ), state_y, true );

    // First, draw the left column
    state_y += ( str_source_h * 2U );
    const char* str_state_id = "Processor ID: ";
    uint32_t state_id_w, _str_h_;
    axk_basicterminal_get_text_size( str_state_id, &state_id_w, &_str_h_ );
    axk_basicterminal_draw_text( str_state_id, padding, state_y, true );

    // TODO
    uint32_t pid = 0U;
    axk_basicterminal_draw_number( (uint64_t) pid, padding + state_id_w, state_y, true );
}


void _panic_print_word( const char* str, size_t len )
{
    if( str == NULL || len == 0UL ) { return; }

    // Determine the word size, and if we should go to the next line
    uint32_t str_w, str_h;
    axk_basicterminal_get_text_size_n( str, len, &str_w, &str_h );
    if( ( str_w + print_x ) > message_w )
    {
        print_y += ( str_h + ( str_h / 4U ) );
        print_x = 0U;
    }

    // If we run out of room, then dont print
    if( ( print_y + str_h ) > message_h ) { return; }

    // Draw the string
    axk_basicterminal_draw_text_n( str, len, print_x + message_x, print_y + message_y, true );
    print_x += str_w;
}

void _panic_print_newline( void )
{
    uint32_t w, h;
    axk_basicterminal_get_text_size( " ", &w, &h );

    print_x = 0U;
    print_y += ( h + ( h / 4U ) );
}


void axk_panic_prints( const char* str )
{
    // Ensure we are in an actual panic state before writing to the screen
    if( !b_panicing || message_x == 0U || message_y == 0U || message_w == 0U || message_h == 0U ) { return; }

    // We are going to loop through the string, word by word, and print within the provided box
    size_t start_word   = 0UL;
    size_t index        = 0UL;

    while( true )
    {
        // Check for special characters
        char c          = *( str + index );
        bool b_space    = false;
        bool b_tab      = false;
        bool b_end      = false;
        bool b_newline  = false;

        switch( c )
        {
            case ' ':
            b_space = true;
            break;
            case '\t':
            b_tab = true;
            break;
            case '\0':
            b_end = true;
            break;
            case '\n':
            b_newline = true;
            break;
        }

        if( b_space || b_tab || b_end || b_newline )
        {
            // We found the end of a word, so lets print it
            if( start_word < index )
            {
                _panic_print_word( str + start_word, index - start_word + ( b_space ? 1UL : 0UL ) );
            }

            if( b_end ) { break; }
            if( b_tab ) { _panic_print_word( "    ", 4 ); }
            if( b_newline ) { _panic_print_newline(); }

            start_word = index + 1UL;
        }

        index++;
    }
}


void axk_panic_printn( uint64_t num )
{
    if( num == 0UL )
    {
        axk_panic_prints( "0" );
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
        axk_panic_prints( str_data );
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


void axk_panic_printh( uint64_t num, bool b_leading_zeros )
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
    axk_panic_prints( str_data );
}


void axk_panic_set_pstate( void* pstate )
{
    ptr_pstate = pstate;
}


__attribute__((noreturn)) void axk_panic_end( void )
{
    axk_halt();
}
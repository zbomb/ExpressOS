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


/*
    axk_panic_helper
    * Arch-specific function
    * Each architecture must implement this, it should stop the execution of all other processors on the system
*/
void axk_panic_helper( void );


void axk_panic_init( void )
{
    axk_spinlock_init( &g_lock );
}


__attribute__((noreturn, noinline)) void axk_panic( const char* str )
{
    // Firstly, disable interrupts, we dont want the kernel to preempt and continue running code after a panic
    // We also need to gain a lock to ensure no other processors can overwrite this panic
    axk_interrupts_disable();
    axk_spinlock_acquire( &g_lock );
    axk_basicterminal_lock();

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
    axk_basicterminal_draw_text( str_source, ( screen_w / 2U ) - ( ( str_source_w + hex64_w ) / 2U ), ( screen_h * 3U ) / 10U, true );
    axk_basicterminal_draw_hex( ret_addr, true, ( screen_w / 2U ) - ( ( str_source_w + hex64_w ) / 2U ) + str_source_w, ( screen_h * 3U ) / 10U, true );

    // TODO: Eventually, it would be nice if we could give out a disassembly of the line of code, or maybe even the code file name and line number
    // but, this could end up being quite complex to implement, or greatly increase the size of the compiled kernel 

    // Draw 'error string header'
    uint32_t err_header_y = ( ( screen_h * 3U ) / 10U ) + ( str_source_h * 3U );
    axk_basicterminal_draw_box( padding, err_header_y, screen_w - ( padding * 2U ), 2, 0 );
    const char* str_err_header = "The error message provided was:";
    uint32_t err_header_w, err_header_h;
    axk_basicterminal_get_text_size( str_err_header, &err_header_w, &err_header_h );
    axk_basicterminal_draw_text( str_err_header, ( screen_w / 2 ) - ( err_header_w / 2 ), err_header_y + str_source_h, true );

    // Draw error message
    axk_basicterminal_draw_text_box( str, padding + padding, err_header_y + ( str_source_h * 3U ), screen_w - ( padding * 4U ), screen_h / 4U, true );
    axk_basicterminal_draw_box( padding, ( screen_h * 3U ) / 5U, screen_w - ( padding * 2U ), 2, false );

    // Draw system state header
    const char* str_state_header = "System State";
    uint32_t state_w, state_h;
    uint32_t state_y = ( ( screen_h * 3U ) / 5U ) + ( str_source_h );
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



    axk_halt();
}


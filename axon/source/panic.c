/*==============================================================
    Axon Kernel - Panic
    2021, Zachary Berry
    axon/source/panic.c
==============================================================*/

#include "axon/panic.h"
#include "axon/arch.h"


void axk_panic( const char* str )
{
    // Disable all interrupts
    axk_disable_interrupts();

    // TODO: Get RIP off stack to let us know where this was called from
#ifdef _X86_64_
    uint64_t ret_addr = (uint64_t) __builtin_extract_return_addr( __builtin_return_address( 0 ) );
#else
    uint64_t ret_addr = 0UL;
#endif

    axk_terminal_lock();
    axk_terminal_clear();
    axk_terminal_fill_fg_color( TC_WHITE );
    axk_terminal_fill_bg_color( TC_RED );
    axk_terminal_set_color_overwrite( false );
    axk_terminal_prints( "=============================> Axon Kernel Error! <=============================" );
    axk_terminal_printnl();
    axk_terminal_printnl();
    axk_terminal_prints( "\tThe kernel encountered an error it couldnt recover from! \n" );
    axk_terminal_printtab();
    axk_terminal_prints( "Panic was called from: " );
    axk_terminal_printh64_lz( ret_addr, true );
    axk_terminal_printnl();

    // TODO: Print the current processor identifier

    axk_terminal_printnl();
    axk_terminal_prints( "\tThe error code thrown was: \n\n" );
    axk_terminal_setfg( TC_LIGHT_CYAN );
    axk_terminal_set_color_overwrite( true );
    axk_terminal_printtab();
    axk_terminal_printtab();
    axk_terminal_prints( str );

    // Stop execution on this thread..
    // TODO: We need to somehow stop all threads

    // Stop this processor
    axk_halt();
}
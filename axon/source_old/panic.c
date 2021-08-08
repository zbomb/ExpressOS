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

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "=============================> Axon Kernel Error! <=============================" );
    axk_basicout_printnl();
    axk_basicout_printnl();
    axk_basicout_prints( "\tThe kernel encountered an error it couldnt recover from! \n" );
    axk_basicout_printtab();
    axk_basicout_prints( "\t\tPanic was called from: " );
    axk_basicout_printh64( ret_addr, true );
    axk_basicout_printnl();

    // TODO: Print the current processor identifier

    axk_basicout_printnl();
    axk_basicout_prints( "\tThe error code thrown was: \n\n" );
    axk_basicout_printtab();
    axk_basicout_printtab();
    axk_basicout_prints( str );

    // Stop execution on this thread..
    // TODO: We need to somehow stop all threads

    // Stop this processor
    axk_halt();
}
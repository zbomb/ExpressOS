/*==============================================================
    Axon Kernel - C Entry Point
    2021, Zachary Berry
    axon/source/entry.c
==============================================================*/

#include "axon/debug_print.h"


void ax_c_main_bsp( void )
{
    axk_terminal_lock();
    axk_terminal_clear();
    axk_terminal_prints( "=====> Main Entry Point!! \n" );
    axk_terminal_unlock();

    while( true ) { __asm__( "hlt" ); }
}


void ax_c_main_ap( void )
{

}
/*==============================================================
    Axon Kernel - C Entry Point
    2021, Zachary Berry
    axon/source/entry.c
==============================================================*/

#include "axon/panic.h"
#include "axon/debug_print.h"
#include "axon/memory/page_mgr.h"
#include "axon/memory/map_mgr.h"
#include "axon/memory/page_alloc.h"
#include "axon/memory/kmap.h"
#include "axon/memory/heap_mgr.h"
#include "axon/memory/kheap.h"
#include "axon/arch.h"
#include "axon/system/interrupts_mgr.h"
#include "axon/system/timers_mgr.h"
#include "axon/system/timers.h"
#include "axon/system/time.h"

#ifdef _X86_64_
#include "axon/arch_x86/acpi_info.h"
#endif


void test_callback( void )
{
    axk_terminal_prints( "========> CALLBACK\n" );
}

void ax_c_main_bsp( void* ptr_info )
{
    axk_terminal_prints( "Kernel: Initializing...\n" );

    /*
        Initialize page allocator
    */
    if( !axk_pagemgr_init() )
    {
        axk_panic( "Kernel: failed to initialize page" );
    }

    /*
        Initialize kernel memory map system
    */
    if( !axk_mapmgr_init() )
    {
        axk_panic( "Kernel: failed to initialize kernel memory map" );
    }

    /*
        Initialize kernel heap
    */
    if( !axk_kheap_init() )
    {
        axk_panic( "Kernel: failed to initialize kernel heap" );
    }

    /*
        Initialize ACPI tables (X86 ONLY)
    */
#ifdef _X86_64_
    if( !axk_x86_acpi_parse() )
    {
        axk_panic( "Kernel: failed to parse ACPI tables" );
    }
#endif

    /*
        Initialize interrupt driver
    */
    if( !axk_interrupts_init() )
    {
        axk_panic( "Kernel: failed to initialize interrupt driver" );
    }

    /*
        Load timer system
    */
    if( !axk_timers_init() )
    {
        axk_panic( "Kernel: failed to initialize timer drivers" );
    }

    while( 1 ) { __asm__( "hlt" ); }

}


void ax_c_main_ap( void )
{

}
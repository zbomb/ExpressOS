/*==============================================================
    Axon Kernel - C Main Function (x86)
    2021, Zachary Berry
    axon/source/arch_x86/main.c
==============================================================*/
#ifdef __x86_64__

#include "axon/kernel/kernel.h"
#include "axon/kernel/boot_params.h"
#include "axon/gfx/basic_terminal.h"
#include "axon/kernel/panic_private.h"
#include "axon/system/sysinfo_private.h"
#include "axon/memory/memory_private.h"
#include "axon/memory/page_allocator.h"

uint64_t page_test[ 100 ];


/*
    Entry Point Function
*/
void axk_x86_main( struct tzero_payload_parameters_t* generic_params, struct tzero_x86_payload_parameters_t* x86_params )
{
    // Initialize the basic terminal system, so we can print to console
    if( !axk_basicterminal_init( generic_params ) )
    {
        generic_params->fn_on_error( "Failed to initialize basic terminal support" );
        axk_halt();
    }

    // Initialize panic spin-lock
    axk_panic_init();

    // Indicate we are ready to take control of the system, this call also will write the memory map into our parameter structure
    generic_params->fn_on_success();

    // Print a header message
    axk_basicterminal_clear();
    axk_basicterminal_prints( "Axon: System control transferred from bootloader, initializing kernel... \n\n" );

    // Next, initialize system counters so we can keep track of various statistics during system runtime
    axk_counters_init();

    // Initialize the physical memory system
    axk_page_allocator_init( generic_params );

    // Lets lock every 10th page in the system
    uint64_t total_page_count = axk_page_count();
    axk_basicterminal_prints( "==> Total number of pages: " );
    axk_basicterminal_printu64( total_page_count );
    axk_basicterminal_printnl();

    for( uint64_t i = 0; i <= ( total_page_count - 1UL ) / 10UL; i++ )
    {
        uint64_t page_id = i * 10UL;
        axk_page_lock( 1UL, &page_id, AXK_PROCESS_KERNEL, AXK_PAGE_TYPE_OTHER, AXK_PAGE_FLAG_NONE );
    }

    // And now, were going to attempt to find a range of 10 pages and see what happens
    if( axk_page_acquire( 10UL, page_test, AXK_PROCESS_KERNEL, AXK_PAGE_TYPE_OTHER, AXK_PAGE_FLAG_PREFER_HIGH ) )
    {
        axk_basicterminal_prints( "==> Page acquire success! List is as follows: \n\t" );
        for( uint64_t i = 0; i < 10UL; i++ )
        {
            axk_basicterminal_printu64( page_test[ i ] );
            axk_basicterminal_printtab();
        }
    }

    axk_basicterminal_printnl();

    if( axk_page_release( 10UL, page_test, AXK_PAGE_FLAG_KERNEL_REL ) )
    {
        axk_basicterminal_prints( "====> Release test passed\n" );
    }
    else
    {
        axk_basicterminal_prints( "====> Release test failed!\n" );
    }

    //if( axk_page_lock( 10UL, page_test, AXK_PROCESS_KERNEL, AXK_PAGE_TYPE_OTHER, AXK_PAGE_FLAG_CLEAR ) )
    //{
    //    axk_basicterminal_prints( "=====> Lock test passed\n" );
    //}
    //else
    //{
    //    axk_basicterminal_prints( "=====> Lock test failed\n" );
    //}

    axk_basicterminal_printnl();

    // 521311
    uint32_t proc;
    uint8_t state, type;

    if( axk_page_status( 521311, &proc, &state, &type ) )
    {
        axk_basicterminal_prints( "====> Status passed.. Process: " );
        axk_basicterminal_printu32( proc );
        axk_basicterminal_prints( "\tType: " );
        switch( type )
        {
            case AXK_PAGE_TYPE_HEAP:
            axk_basicterminal_prints( "HEAP" );
            break;
            case AXK_PAGE_TYPE_IMAGE:
            axk_basicterminal_prints( "IMG" );
            break;
            case AXK_PAGE_TYPE_PAGE_TABLE:
            axk_basicterminal_prints( "PAGETABLE" );
            break;
            case AXK_PAGE_TYPE_SHARED:
            axk_basicterminal_prints( "SHARED" );
            break;
            default:
            axk_basicterminal_prints( "OTHER" );
        }
        axk_basicterminal_prints( "\tState: " );
        switch( state )
        {
            case AXK_PAGE_STATE_ACPI:
            axk_basicterminal_prints( "ACPI" );
            break;
            case AXK_PAGE_STATE_AVAILABLE:
            axk_basicterminal_prints( "AVAIL" );
            break;
            case AXK_PAGE_STATE_BOOTLOADER:
            axk_basicterminal_prints( "BOOTLOADER" );
            break;
            case AXK_PAGE_STATE_LOCKED:
            axk_basicterminal_prints( "LOCKED" );
            break;
            case AXK_PAGE_STATE_RESERVED:
            axk_basicterminal_prints( "RSVD" );
            break;
        }
        axk_basicterminal_printnl();
    }
    else
    {
        axk_basicterminal_prints( "===> Status function failed!\n" );
    }


    while( 1 ) { __asm__( "hlt" ); }
}

#endif
/*==============================================================
    Axon Kernel - C Entry Point
    2021, Zachary Berry
    axon/source/entry.c
==============================================================*/

#include "axon/panic.h"
#include "axon/debug_print.h"
#include "axon/memory/page_alloc_private.h"
#include "axon/memory/kmap_private.h"
#include "axon/memory/page_alloc.h"
#include "axon/memory/kmap.h"
#include "axon/memory/kheap_private.h"
#include "axon/memory/kheap.h"
#include "axon/arch.h"
#include "axon/system/interrupts_private.h"
#include "axon/system/timers_private.h"
#include "axon/system/timers.h"
#include "axon/system/time.h"
#include "axon/memory/atomics.h"

// Testing
#include "axon/library/vector.h"
#include "axon/library/rbtree.h"

#ifdef _X86_64_
#include "axon/arch_x86/acpi_info.h"
#endif

/*
    State
*/
static struct axk_atomic_flag_t g_ap_wait_flag;

static uint64_t callback_counter = 0;

bool test_callback( void )
{
    callback_counter++;
    return false;
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

    /*
        Start the Aux Processors
    */
    axk_atomic_clear_flag( &g_ap_wait_flag, MEMORY_ORDER_SEQ_CST );

    uint32_t proc_count;
    if( !axk_start_aux_processors( &proc_count ) )
    {
        axk_panic( "Kernel: failed to start other processors" );
    }

    /*
        Syncronize System Clock
    */
    if( !axk_timers_bsp_sync() )
    {
        axk_panic( "Kernel: failed to syncronize system clock" );
    }

    /*
        Do a test....
    */
    struct axk_rbtree_t tree;
    axk_rbtree_init( &tree );

    axk_rbtree_insert( &tree, 100UL, 50UL, false );
    axk_rbtree_insert( &tree, 110UL, 20UL, false );
    axk_rbtree_insert( &tree, 120UL, 10UL, false );
    axk_rbtree_insert( &tree, 101UL, 50UL, false );
    axk_rbtree_insert( &tree, 111UL, 20UL, false );
    axk_rbtree_insert( &tree, 121UL, 10UL, false );

    struct axk_rbtree_iterator_t iter;
    axk_rbtree_iterator_init( &iter );
    axk_rbtree_begin( &tree, &iter );

    axk_terminal_prints( "===> Begin Key: " );
    axk_terminal_printu64( iter.node->key );
    axk_terminal_printnl();
    
    axk_rbtree_erase( &tree, &iter );
    axk_rbtree_erase( &tree, &iter );

    axk_terminal_prints( "===> Begin Key: " );
    axk_terminal_printu64( iter.node->key );
    axk_terminal_printnl();

    do
    {
        axk_terminal_prints( "====> Key: " );
        axk_terminal_printu64( iter.node->key );
        axk_terminal_prints( "  Value: " );
        axk_terminal_printu64( iter.node->inline_data );
        axk_terminal_printnl();

    } while ( axk_rbtree_next( &iter ) );
    

    while( 1 ) { __asm__( "hlt" ); }

}


void ax_c_main_ap( void )
{
    // Wait for the APs to be released
    while( axk_atomic_test_flag( &g_ap_wait_flag, MEMORY_ORDER_SEQ_CST ) )
    {
        #ifdef _X86_64_
        __asm__( "pause" );
        #endif
    }

    /*
        Initialize Interrupts
    */
    if( !axk_interrupts_init_aux() )
    {
        axk_panic( "Kernel: failed to initialize interrupts on aux processor" );
    }

    /*
        Syncronize System Clock
    */
    if( !axk_timers_ap_sync() )
    {
        axk_panic( "Kernel: failed to syncronize system clock on aux processor" );
    }


    // We cant actually return from this function, there is no return address on the stack
    axk_halt();
}
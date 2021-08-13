/*==============================================================
    Axon Kernel - C Main Function (x86)
    2021, Zachary Berry
    axon/source/arch_x86/main.c
==============================================================*/
#ifdef __x86_64__

#include "axon/kernel/kernel.h"
#include "axon/kernel/boot_params.h"
#include "axon/gfx/basic_terminal.h"
#include "axon/kernel/panic.h"


void axk_x86_main( struct tzero_payload_parameters_t* generic_params, struct tzero_x86_payload_parameters_t* x86_params )
{
    // Initialize the basic terminal system, so we can print to console
    if( !axk_basicterminal_init( generic_params ) )
    {
        generic_params->fn_on_error( "Failed to initialize basic terminal support" );
        axk_halt();
    }

    // Indicate we are ready to take control of the system, this call also will write the memory map into our parameter structure
    generic_params->fn_on_success();

    // Print a header message
    axk_basicterminal_clear();
    axk_basicterminal_prints( "Axon: System control transferred from bootloader, initializing kernel... \n\n" );

    // Next, initialize system counters so we can keep track of various statistics during system runtime
    axk_counters_init();

    // Initialize the physical memory system
    

    while( 1 ) { __asm__( "hlt" ); }
}

#endif
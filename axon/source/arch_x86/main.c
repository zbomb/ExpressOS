/*==============================================================
    Axon Kernel - C Main Function (x86)
    2021, Zachary Berry
    axon/source/arch_x86/main.c
==============================================================*/
#ifdef __x86_64__

#include "axon/kernel/kernel.h"
#include "axon/kernel/boot_params.h"
#include "axon/gfx/basic_terminal_private.h"
#include "axon/kernel/panic_private.h"
#include "axon/system/sysinfo_private.h"
#include "axon/memory/memory_private.h"
#include "axon/memory/page_allocator.h"

uint64_t page_test[ 100 ];

/*
    Helper Macro
*/
#define AXK_FIX_PTR( _ptr_, _ty_ ) _ptr_ = (_ty_)( (uint64_t)( _ptr_ ) + AXK_KERNEL_VA_PHYSICAL )


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

    // Initiailize the memory map 
    axk_kmap_init( generic_params );

    // Now, we need to update the parameter structures since the UEFI mappings are now gone
    AXK_FIX_PTR( generic_params, struct tzero_payload_parameters_t* );
    AXK_FIX_PTR( x86_params, struct tzero_x86_payload_parameters_t* );
    AXK_FIX_PTR( generic_params->memory_map.list, struct tzero_memory_entry_t* );
    AXK_FIX_PTR( generic_params->available_resolutions, struct tzero_resolution_t* );
    

    while( 1 ) { __asm__( "hlt" ); }
}

#endif
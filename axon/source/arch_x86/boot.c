/*==============================================================
    Axon Kernel - x86 Specific Boot Code
    2021, Zachary Berry
    axon/arch_x86/boot/boot.c
==============================================================*/

#ifdef _X86_64_
#include "axon/arch.h"
#include "axon/arch_x86/acpi_info.h"
#include "axon/system/interrupts.h"
#include "axon/memory/atomics.h"
#include "axon/system/processor_info.h"
#include "axon/panic.h"
#include "axon/system/timers.h"
#include "axon/debug_print.h"
#include "string.h"
#include "stdlib.h"

/*
    State
*/
static bool g_ap_init = false;

/*
    ASM Functions/Variables
*/
extern uint64_t axk_get_ap_code_begin( void );
extern uint64_t axk_get_ap_code_size( void );
extern uint32_t axk_ap_counter;
extern uint64_t axk_ap_stack;
extern uint64_t axk_ap_wait_flag;


bool axk_start_aux_processors( uint32_t* out_cpu_count )
{
    // Ensure this hasnt already been called
    if( g_ap_init || out_cpu_count == NULL ) { return false; }
    g_ap_init = true;

    struct axk_x86_acpi_info_t* ptr_acpi = axk_x86_acpi_get();
    if( ptr_acpi == NULL ) { return false; }

    uint32_t cpu_count  = axk_processor_get_count();
    uint32_t bsp_id     = axk_processor_get_boot_id();
    ( *out_cpu_count )     = 1;

    if( cpu_count <= 1 )
    {
        return true;
    }

    uint64_t init_addr      = AXK_AP_INIT_ADDRESS + AXK_KERNEL_VA_PHYSICAL;
    uint64_t code_begin     = axk_get_ap_code_begin();
    uint64_t code_size      = axk_get_ap_code_size();

    // Ensure the init code isnt larger than a single page
    if( code_size > 0x1000UL ) 
    {
        axk_panic( "Boot (x86): Aux Processor init code is larger than one page?" );
    }

    // Copy the code to the physical memory location in low memory
    memcpy( (void*) init_addr, (void*) code_begin, code_size );

    // Setup some state
    axk_ap_wait_flag    = 0;
    axk_ap_counter      = 1;

    // Loop through each processor and start it
    for( uint32_t i = 0; i < ptr_acpi->lapic_count; i++ )
    {
        struct axk_x86_lapic_info_t* ptr_info = ptr_acpi->lapic_list + i;
        if( ptr_info->id == bsp_id ) { continue; }

        // Send INIT signal
        struct axk_interprocessor_interrupt_t init;

        init.target_processor       = (uint32_t) ptr_info->processor;
        init.interrupt_vector       = 0;
        init.delivery_mode          = AXK_IPI_DELIVERY_MODE_INIT;
        init.b_deassert             = false;
        init.b_wait_for_receipt     = true;

        axk_interrupts_clear_error();
        if( !axk_interrupts_send_ipi( &init ) )
        {
            return false;
        }

        // TODO: Check for error
        uint32_t init_err = axk_interrupts_get_error();

        // Wait for 10ms
        axk_delay( 10000000UL );

        // Allocate a kernel stack for this processor
        axk_ap_stack = (uint64_t) malloc( AXK_KERNEL_STACK_SIZE ) + AXK_KERNEL_STACK_SIZE;
        uint32_t prev_counter = axk_ap_counter;

        // Send SIPI signal
        init.target_processor       = (uint32_t) ptr_info->processor;
        init.interrupt_vector       = 0x08;
        init.delivery_mode          = AXK_IPI_DELIVERY_MODE_START;
        init.b_deassert             = false;
        init.b_wait_for_receipt     = true;

        axk_interrupts_clear_error();
        if( !axk_interrupts_send_ipi( &init ) )
        {
            return false;
        }

        // TODO: Check for errors
        init_err = axk_interrupts_get_error();

        // Wait for 2ms and check if the processor started up as expected
        axk_delay( 2000000UL );

        if( axk_ap_counter == prev_counter )
        {
            // Try one more time to start the processor
            axk_interrupts_clear_error();
            if( !axk_interrupts_send_ipi( &init ) )
            {
                return false;
            }

            // TODO: Check for error
            init_err = axk_interrupts_get_error();
            
            // Wait for a full second, and check again
            axk_delay( 1000000000UL );

            if( axk_ap_counter == prev_counter )
            {
                return false;
            }
        }

        // Processor was started successfully
        ( *out_cpu_count )++;
    }

    // Print debug message
    axk_terminal_lock();
    axk_terminal_prints( "Boot: Started all available processors, there are now " );
    axk_terminal_printu32( *out_cpu_count );
    axk_terminal_prints( " processors running\n" );
    axk_terminal_unlock();

    // And now, release the processors
    axk_ap_wait_flag = 1;
    return true;
}


#endif
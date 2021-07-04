/*==============================================================
    Axon Kernel - Interrupts
    2021, Zachary Berry
    axon/source/system/interrupts.c
==============================================================*/

#include "axon/system/interrupts.h"
#include "axon/system/interrupts_mgr.h"
#include "axon/library/spinlock.h"
#include "axon/memory/atomics.h"
#include "axon/panic.h"

/*
    Structures
*/
struct axk_interrupt_handler_t
{
    uint32_t process;
    struct axk_atomic_pointer_t callback;

    // Even though we use a lock to guard modifications to the list, we still use an atomic pointer
    // to store the function pointer, because when an interrupt is raised, we dont want to wait for a spinlock
    // to be acquired when calling an interrupt callback. Although the lock wont be held often, so it might be overkill
};

/*
    State
*/
static struct axk_interrupt_handler_t g_handlers[ AXK_MAX_INTERRUPT_HANDLERS ];
static bool g_init = false;
static struct axk_spinlock_t g_lock;

/*
    Function Implementations
*/
void axk_interrupts_init_state( void )
{
    // Initialize the spinlock
    axk_spinlock_init( &g_lock );

    // Loop through, and initialize handler entries
    for( uint8_t i = 0; i < AXK_MAX_INTERRUPT_HANDLERS; i++ )
    {
        g_handlers[ i ].process = AXK_PROCESS_INVALID;
        axk_atomic_store_pointer( &( g_handlers[ i ].callback ), NULL, MEMORY_ORDER_SEQ_CST );
    }
}


void axk_interrupts_invoke( uint8_t vec )
{
    // Validate the interrupt number
    if( vec >= AXK_MAX_INTERRUPT_HANDLERS )
    {
        axk_panic( "Interrupts: interrupt raised with an out-of-bounds interrupt number" );
    }

    // Find the current callback bound to this vector
    void* vcallback = axk_atomic_load_pointer( &( g_handlers[ vec ].callback ), MEMORY_ORDER_SEQ_CST );
    bool b_sent_eoi = false;

    if( vcallback != NULL )
    {
        // Invoke the callback
        bool( *fcallback )(uint8_t) = (bool(*)(uint8_t))( vcallback );
        b_sent_eoi = fcallback( vec );
    }

    if( !b_sent_eoi )
    {
        axk_interrupts_signal_eoi();
    }
}


bool axk_interrupts_acquire_handler( uint32_t process, bool( *func_ptr )( uint8_t ), uint8_t* out_vec )
{
    // Validate parameters
    if( out_vec == NULL || process == AXK_PROCESS_INVALID ) { return false; }

    // Acquire lock
    axk_spinlock_acquire( &g_lock );

    // Look for an available interrupt handler entry
    struct axk_interrupt_handler_t* ptr_entry = NULL;
    uint8_t index = 0;
    
    for( uint8_t i = 0; i < AXK_MAX_INTERRUPT_HANDLERS; i++ )
    {
        if( g_handlers[ i ].process == AXK_PROCESS_INVALID )
        {
            ptr_entry   = g_handlers + i;
            index       = i;

            break;
        }
    }

    // Ensure we didnt run out of interrupt handlers...
    if( ptr_entry == NULL )
    {
        axk_panic( "Interrupts: ran out of available interrupt handlers" );
    }

    // Update entry and release lock
    ptr_entry->process = process;

    axk_atomic_store_pointer( &( ptr_entry->callback ), (void*)func_ptr, MEMORY_ORDER_SEQ_CST );
    axk_spinlock_release( &g_lock );

    *out_vec = index;
    return true;
}


bool axk_interrupts_lock_handler( uint32_t process, bool( *func_ptr )( uint8_t ), uint8_t vec )
{
    // Validate parameters
    if( process == AXK_PROCESS_INVALID || vec >= AXK_MAX_INTERRUPT_HANDLERS ) { return false; }

    // Acquire lock
    axk_spinlock_acquire( &g_lock );

    // Check if the target handler is already owned
    struct axk_interrupt_handler_t* ptr_entry = g_handlers + vec;
    if( ptr_entry->process != AXK_PROCESS_INVALID ) 
    { 
        axk_spinlock_release( &g_lock );
        return false; 
    }

    // Update the entry and relaese the lock
    ptr_entry->process = process;
    axk_atomic_store_pointer( &( ptr_entry->callback ), (void*) func_ptr, MEMORY_ORDER_SEQ_CST );
    axk_spinlock_release( &g_lock );

    return true;
}


void axk_interrupts_release_handler( uint8_t vec )
{
    // Validate parameter
    if( vec >= AXK_MAX_INTERRUPT_HANDLERS ) { return; }

    // Acquire lock while modifying state
    axk_spinlock_acquire( &g_lock );

    struct axk_interrupt_handler_t* ptr_entry = g_handlers + vec;

    ptr_entry->process      = AXK_PROCESS_INVALID;
    axk_atomic_store_pointer( &( ptr_entry->callback ), NULL, MEMORY_ORDER_SEQ_CST );

    axk_spinlock_release( &g_lock );
}


bool axk_interrupts_update_handler( uint8_t vec, bool( *func_ptr )( uint8_t ) )
{
    // Validate parameters
    if( vec >= AXK_MAX_INTERRUPT_HANDLERS ) { return false; }

    // Acquire lock whie modifying state
    axk_spinlock_acquire( &g_lock );

    struct axk_interrupt_handler_t* ptr_entry = g_handlers + vec;
    if( ptr_entry->process == AXK_PROCESS_INVALID )
    {
        axk_spinlock_release( &g_lock );
        return false;
    }

    axk_atomic_store_pointer( &( ptr_entry->callback ), (void*) func_ptr, MEMORY_ORDER_SEQ_CST );
    axk_spinlock_release( &g_lock );

    return true;
}


uint8_t axk_interrupts_release_process_handlers( uint32_t process )
{
    // Validate parameter
    if( process == AXK_PROCESS_INVALID ) { return 0; }
    uint8_t count = 0;

    // Acuiqre lock to modify state
    axk_spinlock_acquire( &g_lock );

    for( uint8_t i = 0; i < AXK_MAX_INTERRUPT_HANDLERS; i++ )
    {
        if( g_handlers[ i ].process == process )
        {
            g_handlers[ i ].process     = AXK_PROCESS_INVALID;
            axk_atomic_store_pointer( &( g_handlers[ i ].callback ), NULL, MEMORY_ORDER_SEQ_CST );

            count++;
        }
    }

    axk_spinlock_release( &g_lock );
    return count;
}


bool axk_interrupts_get_handler_info( uint8_t vec, bool( **out_func )( uint8_t ), uint32_t* out_process )
{
    // Validate parameters
    if( vec >= AXK_MAX_INTERRUPT_HANDLERS || out_func == NULL || out_process == NULL ) { return false; }

    // Acquire lock to read state
    axk_spinlock_acquire( &g_lock );

    struct axk_interrupt_handler_t* ptr_entry = g_handlers + vec;
    if( ptr_entry->process == AXK_PROCESS_INVALID ) 
    {
        axk_spinlock_release( &g_lock );
        return false;
    }

    *out_func       = (bool(*)(uint8_t))( axk_atomic_load_pointer( &( ptr_entry->callback ), MEMORY_ORDER_SEQ_CST ) );
    *out_process    = ptr_entry->process;

    axk_spinlock_release( &g_lock );
    return true;
}


void axk_interrupts_signal_eoi( void )
{
    struct axk_interrupt_driver_t* driver = axk_interrupts_get();
    driver->signal_eoi( driver );
}


bool axk_interrupts_send_ipi( struct axk_interprocessor_interrupt_t* ipi )
{
    struct axk_interrupt_driver_t* driver = axk_interrupts_get();
    return driver->send_ipi( driver, ipi );
}


bool axk_interrupts_set_ext_routing( struct axk_external_interrupt_routing_t* routing )
{
    struct axk_interrupt_driver_t* driver = axk_interrupts_get();
    return driver->set_external_routing( driver, routing );
}


bool axk_interrupts_get_ext_routing( uint32_t ext_num, struct axk_external_interrupt_routing_t* out_routing )
{
    struct axk_interrupt_driver_t* driver = axk_interrupts_get();
    return driver->get_external_routing( driver, ext_num, out_routing );
}


uint32_t axk_interrupts_get_error( void )
{
    struct axk_interrupt_driver_t* driver = axk_interrupts_get();
    return driver->get_error( driver );
}


void axk_interrupts_clear_error( void )
{
    struct axk_interrupt_driver_t* driver = axk_interrupts_get();
    driver->clear_error( driver );
}


uint32_t axk_interrupts_get_ext_number( uint8_t bus, uint8_t irq )
{
    struct axk_interrupt_driver_t* driver = axk_interrupts_get();
    return driver->get_ext_int( driver, bus, irq );
}
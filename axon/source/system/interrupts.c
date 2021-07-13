/*==============================================================
    Axon Kernel - Interrupts
    2021, Zachary Berry
    axon/source/system/interrupts.c
==============================================================*/

#include "axon/system/interrupts.h"
#include "axon/system/interrupts_private.h"
#include "axon/library/spinlock.h"
#include "axon/memory/atomics.h"
#include "axon/panic.h"
#include "stdlib.h"

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

struct axk_interrupt_external_t
{
    uint32_t process;
    uint32_t global_interrupt;
};

/*
    State
*/
static struct axk_interrupt_handler_t g_handlers[ AXK_MAX_INTERRUPT_HANDLERS ];
static bool g_init = false;
static struct axk_spinlock_t g_lock;
static struct axk_interrupt_external_t* g_external_routings;
static uint32_t g_external_routings_count;

/*
    Function Implementations
*/
void axk_interrupts_init_state( void )
{
    // Initialize the spinlocks
    axk_spinlock_init( &g_lock );

    // Loop through, and initialize handler entries
    for( uint8_t i = 0; i < AXK_MAX_INTERRUPT_HANDLERS; i++ )
    {
        g_handlers[ i ].process = AXK_PROCESS_INVALID;
        axk_atomic_store_pointer( &( g_handlers[ i ].callback ), NULL, MEMORY_ORDER_SEQ_CST );
    }

    // Get the list of available external interrupt routings
    struct axk_interrupt_driver_t* ptr_driver = axk_interrupts_get();

    g_external_routings_count = ptr_driver->get_available_external_routings( ptr_driver, NULL );
    if( g_external_routings_count == 0U )
    {
        g_external_routings = NULL;
    }
    else
    {
        g_external_routings     = (struct axk_interrupt_external_t*) calloc( g_external_routings_count, sizeof( struct axk_interrupt_external_t ) );
        uint32_t* list          = (uint32_t*) calloc( g_external_routings_count, sizeof( uint32_t) );

        ptr_driver->get_available_external_routings( ptr_driver, list );

        for( uint32_t i = 0; i < g_external_routings_count; i++ )
        {
            g_external_routings[ i ].process            = AXK_PROCESS_INVALID;
            g_external_routings[ i ].global_interrupt   = list[ i ];
        }

        free( list );
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


uint8_t axk_interrupts_release_process_resources( uint32_t process )
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

    for( uint32_t i = 0; i < g_external_routings_count; i++ )
    {
        if( g_external_routings[ i ].process == process )
        {
            g_external_routings[ i ].process = AXK_PROCESS_INVALID;
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


bool axk_interrupts_acquire_external( uint32_t process, struct axk_external_interrupt_routing_t* routing )
{
    if( process == AXK_PROCESS_INVALID || routing == NULL ) { return false; }

    // Acquire spinlock
    axk_spinlock_acquire( &g_lock );

    // Look for an available external interrupt
    bool b_valid    = false;

    for( uint32_t i = 0; i < g_external_routings_count; i++ )
    {
        if( g_external_routings[ i ].process == AXK_PROCESS_INVALID )
        {
            b_valid                         = true;
            routing->global_interrupt    = g_external_routings[ i ].global_interrupt;

            g_external_routings[ i ].process = process;
            break;
        }
    }

    axk_spinlock_release( &g_lock );
    if( !b_valid )
    {
        // DEBUG
        axk_panic( "Interrupts: Ran out of external interrupt routings" );
        return false;
    }

    struct axk_interrupt_driver_t* driver = axk_interrupts_get();
    if( !driver->set_external_routing( driver, routing ) )
    {
        axk_panic( "Interrupts: Driver wouldnt allow external interrupt routing to be updated" );
    }

    return true;
}


bool axk_interrupts_acquire_external_clamped( uint32_t process, struct axk_external_interrupt_routing_t* routing, uint32_t* allowed, uint32_t allowed_count )
{
    if( process == AXK_PROCESS_INVALID || routing == NULL || allowed == NULL || allowed_count == 0U ) { return false; }

    // Acquire spinlock to modify state
    axk_spinlock_acquire( &g_lock );

    // Loop through allowed vectors, then find the corresponding entry and check if we can acquire them
    bool b_valid = false;

    for( uint32_t i = 0; i < allowed_count; i++ )
    {
        for( uint32_t j = 0; j < g_external_routings_count; j++ )
        {
            if( g_external_routings[ j ].process == AXK_PROCESS_INVALID )
            {
                g_external_routings[ j ].process    = process;
                routing->global_interrupt           = g_external_routings[ j ].global_interrupt;

                b_valid = true;
                break;
            }
        }
    }

    // Release lock and check for success
    axk_spinlock_release( &g_lock );
    if( !b_valid ) 
    {
        return false;
    }

    // Update the external routing in the driver
    struct axk_interrupt_driver_t* driver = axk_interrupts_get();
    if( !driver->set_external_routing( driver, routing ) )
    {
        axk_panic( "Interrupts: Driver wouldnt allow external interrupt routing to be updated" );
    }

    return true;
}


bool axk_interrupts_lock_external( uint32_t process, struct axk_external_interrupt_routing_t* routing, bool b_overwrite )
{
    if( process == AXK_PROCESS_INVALID || routing == NULL ) { return false; }

    // Acquire lock
    axk_spinlock_acquire( &g_lock );
    
    bool b_valid = false;
    for( uint32_t i = 0; i < g_external_routings_count; i++ )
    {
        if( g_external_routings[ i ].global_interrupt == routing->global_interrupt )
        {
            b_valid                             = true;
            g_external_routings[ i ].process    = process;
            break;
        }
    }

    axk_spinlock_release( &g_lock );
    if( !b_valid )
    {
        return false;
    }

    struct axk_interrupt_driver_t* driver = axk_interrupts_get();
    if( !driver->set_external_routing( driver, routing ) )
    {
        axk_panic( "Interrupts: Driver wouldnt allow external interrupt routing to be updated" );
    }
    return true;
}


void axk_interrupts_release_external( uint32_t vector )
{
    // Acquire spinlock
    axk_spinlock_acquire( &g_lock );

    // Look for this global interrupt
    struct axk_interrupt_driver_t* driver = axk_interrupts_get();

    for( uint32_t i = 0; i < g_external_routings_count; i++ )
    {
        if( g_external_routings[ i ].global_interrupt == vector )
        {
            driver->clear_external_routing( driver, vector );
            g_external_routings[ i ].process = AXK_PROCESS_INVALID;

            break;
        }
    }

    axk_spinlock_release( &g_lock );
}


bool axk_interrupts_update_external( uint32_t vector, struct axk_external_interrupt_routing_t* routing )
{
    struct axk_interrupt_driver_t* driver = axk_interrupts_get();
    return( routing == NULL ?   driver->clear_external_routing( driver, vector ) : 
                                driver->set_external_routing( driver, routing ) );
}


bool axk_interrupts_get_external( uint32_t vector, uint32_t* out_process, struct axk_external_interrupt_routing_t* out_routing )
{
    // Some of the info needed is from the driver, and some of the info needed is from our state
    if( out_process == NULL || out_routing == NULL ) { return false; }

    // Lock state
    axk_spinlock_acquire( &g_lock );
    
    bool b_valid = false;
    for( uint32_t i = 0; i < g_external_routings_count; i++ )
    {
        if( g_external_routings[ i ].global_interrupt == vector )
        {
            *out_process = g_external_routings[ i ].process;
            b_valid = true;
            break;
        }
    }

    axk_spinlock_release( &g_lock );

    if( b_valid )
    {
        struct axk_interrupt_driver_t* driver = axk_interrupts_get();
        return driver->get_external_routing( driver, vector, out_routing );
    }
    else
    {
        return false;
    }
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
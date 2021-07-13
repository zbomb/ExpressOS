/*==============================================================
    Axon Kernel - Interrupt System
    2021, Zachary Berry
    axon/public/axon/system/interrupts.h
==============================================================*/

#pragma once
#include "axon/config.h"

/*
    axk_ipi_delivery_mode_t (ENUM)
    * The mode to use when sending an IPI
    * TODO: This is solely based on x86, how to make cross platform?
*/
enum axk_ipi_delivery_mode_t
{
    AXK_IPI_DELIVERY_MODE_NORMAL    = 0,
    AXK_IPI_DELIVERY_MODE_INIT      = 3,
    AXK_IPI_DELIVERY_MODE_START     = 4
};

/*
    axk_interprocessor_interrupt_t (STRUCT)
    * Contains parameters needed to send an interrupt between processors
*/
struct axk_interprocessor_interrupt_t
{
    uint32_t target_processor;
    uint8_t interrupt_vector;
    enum axk_ipi_delivery_mode_t delivery_mode;
    bool b_deassert;
    bool b_wait_for_receipt;
};

/*
    axk_external_interrupt_routing_t (STRUCT)
    * Info about how an external interrupt is routed to a processor
*/
struct axk_external_interrupt_routing_t
{
    uint32_t global_interrupt;
    uint8_t local_interrupt;
    bool b_low_priority;
    bool b_active_low;
    bool b_level_triggered;
    bool b_masked;
    uint32_t target_processor;
};

/*
    axk_interrupts_acquire_handler
    * Finds an available handler to acquire ownership of
    * Will not allow existing handlers to be overwritten
*/
bool axk_interrupts_acquire_handler( uint32_t process, bool( *func_ptr )( uint8_t ), uint8_t* out_vec );

/*
    axk_interrupts_lock_handler
    * Locks a specific interrupt handler number
    * Should be used sparingly, by the kernel systems
*/
bool axk_interrupts_lock_handler( uint32_t process, bool( *func_ptr )( uint8_t ), uint8_t vec );

/*
    axk_interrupts_release_handler
    * Releases a handler that is locked/acquired by a process
    * Also will unbind the callback
*/
void axk_interrupts_release_handler( uint8_t vec );

/*
    axk_interrupts_update_handler
    * Updates the callback associated with a handler
*/
bool axk_interrupts_update_handler( uint8_t vec, bool( *func_ptr )( uint8_t ) );

/* 
    axk_interrupts_release_process_resources
    * Releases all handlers associated with the specified process
    * Also releases all external routings set by the process
    * Returns the number of released handlers
*/
uint8_t axk_interrupts_release_process_resources( uint32_t process );

/*
    axk_interrupts_get_handler_info
    * Gets information about a specific handler
*/
bool axk_interrupts_get_handler_info( uint8_t vec, bool( **out_func )( uint8_t ), uint32_t* out_process );

/*
    axk_interrupts_signal_eoi
    * Signals to the interrupt driver that an interrupt is done being serviced
*/
void axk_interrupts_signal_eoi( void );

/*
    axk_interrupts_send_ipi
    * Sends an inter-processor interrupt through the interrupt driver
*/
bool axk_interrupts_send_ipi( struct axk_interprocessor_interrupt_t* ipi );

/*
    axk_interrupts_acquire_external
    * Acquires an available external interrupt vector to assign a routing to
    * If you need the assigned vector, its stored in 'routing.global_interrupt'
*/
bool axk_interrupts_acquire_external( uint32_t process, struct axk_external_interrupt_routing_t* routing );

/*
    axk_interrupt_acquire_external_clamped
    * Acquires an available external interrupt vector in the provided range, to assign routing to
    * If you need the assigned vector, its stored in 'routing.global_interrupt'
*/
bool axk_interrupts_acquire_external_clamped( uint32_t process, struct axk_external_interrupt_routing_t* routing, uint32_t* allowed, uint32_t allowed_count );

/*
    axk_interrupts_lock_external
    * Locks a specific external interrupt vector to assign routing to
    * Will overwrite any existing owner of this vector if 'b_overwrite' is true
*/
bool axk_interrupts_lock_external( uint32_t process, struct axk_external_interrupt_routing_t* routing, bool b_overwrite );

/*
    axk_interrupts_release_external
    * Releases ownership of an external interrupt vector previously acquired using any of the following:
        'axk_interrupts_acquire_external'
        'axk_interrupts_acquire_external_clamped'
        'axk_interrupts_lock_external'
    * Will also clear the stored routing
*/
void axk_interrupts_release_external( uint32_t vector );

/*
    axk_interrupts_update_external
    * Updates an external interrupt routing with new information
    * Ensure this interrupt vector is already 'owned' by the calling process!
    * You can clear the current routing by passing 'routing' as NULL
    * The global vector is determined based on 'uint32_t vector', and NOT 'routing.global_interrupt'
*/
bool axk_interrupts_update_external( uint32_t vector, struct axk_external_interrupt_routing_t* routing );

/*
    axk_interrupts_get_external
    * Gets information about an external interrupt routing, including the process that holds ownership
*/
bool axk_interrupts_get_external( uint32_t vector, uint32_t* out_process, struct axk_external_interrupt_routing_t* out_routing );

/*
    axk_interrupts_get_error
    * Gets the pending error from the interrupt driver
*/
uint32_t axk_interrupts_get_error( void );

/*
    axk_interrupts_clear_error
    * Clears any pending errors from the interrupt driver
*/
void axk_interrupts_clear_error( void );

/*
    axk_interrupts_get_ext_number
    * Gets external interrupt number from a specific bus
    * TODO: Might not implement on all platforms?
*/
uint32_t axk_interrupts_get_ext_number( uint8_t bus, uint8_t irq );

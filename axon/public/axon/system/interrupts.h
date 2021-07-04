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
    axk_interrupts_release_process_handlers
    * Releases all handlers associated with the specified process
    * Returns the number of released handlers
*/
uint8_t axk_interrupts_release_process_handlers( uint32_t process );

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
    axk_interrupts_set_ext_routing
    * Sets extenral interrupt routing through the interrupt driver
*/
bool axk_interrupts_set_ext_routing( struct axk_external_interrupt_routing_t* routing );

/*
    axk_interrupts_get_ext_routing
    * Gets external interrupt routing through the interrupt driver
*/
bool axk_interrupts_get_ext_routing( uint32_t ext_num, struct axk_external_interrupt_routing_t* out_routing );

/*
    axk_interrupts_get_error
    * Gets the pending error from the interrupt driver
*/
uint32_t axk_interrupts_get_error( void );

/*
    axk_interrupts_clear_error
    * Clears any pending errors from the interrupt driver
*/
void axk_interrupt_clear_error( void );

/*
    axk_interrupts_get_ext_number
    * Gets external interrupt number from a specific bus
    * TODO: Might not implement on all platforms?
*/
uint32_t axk_interrupts_get_ext_number( uint8_t bus, uint8_t irq );

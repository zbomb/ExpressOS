/*==============================================================
    Axon Kernel - Interrupt Managment
    2021, Zachary Berry
    axon/private/axon/system/interrupts_mgr.h
==============================================================*/

#pragma once
#include "axon/config.h"
#include "axon/system/interrupts.h"


/*
    axk_interrupt_driver_type (ENUM)
*/
enum axk_interrupt_driver_type
{
    INTERRUPT_DRIVER_X86_XAPIC      = 0,
    INTERRUPT_DRIVER_X86_X2APIC     = 1
};

/*
    axk_interrupt_driver_t (STRUCT)
    * Private structure
    * Interface
    * Holds function pointers for the current interrupt driver
*/
struct axk_interrupt_driver_t
{
    bool( *init )( struct axk_interrupt_driver_t* );
    bool( *aux_init )( struct axk_interrupt_driver_t* );
    void( *signal_eoi )( struct axk_interrupt_driver_t* );
    bool( *send_ipi )( struct axk_interrupt_driver_t*, struct axk_interprocessor_interrupt_t* );
    bool( *set_external_routing )( struct axk_interrupt_driver_t*, struct axk_external_interrupt_routing_t* );
    bool ( *get_external_routing )( struct axk_interrupt_driver_t*, uint32_t, struct axk_external_interrupt_routing_t* );
    uint32_t( *get_error )( struct axk_interrupt_driver_t* );
    void( *clear_error )( struct axk_interrupt_driver_t* );
    uint32_t( *get_ext_int )( struct axk_interrupt_driver_t*, uint8_t, uint8_t );
};


/*
    axk_interrupts_init
    * Private function
    * Create the correct version of the interrupt driver
    * Arch-Specific implementation
*/
bool axk_interrupts_init( void );

/*
    axk_interrupts_init_aux
    * Private function
    * Initializes interrupts on an aux processor
    * Arch-Specific implementation
*/
bool axk_interrupts_init_aux( void );

/*
    axk_interrupts_init_state
    * Private function
    * Initializes the interrupt system state
    * Arch-independent implementation (interrupts.c)
*/
void axk_interrupts_init_state( void );

/*
    axk_interrupts_invoke
    * Private function
    * Invokes the execution of an interrupt handler
*/
void axk_interrupts_invoke( uint8_t vec );

/*
    axk_interrupts_get
    * Private function
    * Gets a pointer to the interrupt driver function table
    * Arch-Specific implementation
*/
struct axk_interrupt_driver_t* axk_interrupts_get( void );

/*
    axk_interrupts_get_type
    * Private Function
    * Gets an enum describing what interrupt driver is laoded
*/
enum axk_interrupt_driver_type axk_interrupts_get_type( void );
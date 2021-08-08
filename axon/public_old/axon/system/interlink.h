/*==============================================================
    Axon Kernel - Interlink Message System
    2021, Zachary Berry
    axon/public/axon/system/interlink.h
==============================================================*/

#pragma once
#include "axon/config.h"
#include "axon/memory/atomics.h"

/*
    axk_interlink_message_t (STRUCT)
    * Structure that contains info passed between processors using interlink message passing
*/
struct axk_interlink_message_t
{
    uint32_t type;
    uint32_t param;
    uint32_t flags;
    uint32_t source_cpu;
    uint64_t size;
    void* body;

    struct axk_atomic_uint32_t data_counter;

};

enum axk_interlink_error_t
{
    AXK_INTERLINK_ERROR_NONE                = 0,
    AXK_INTERLINK_ERROR_INVALID_TARGET      = 1,
    AXK_INTERLINK_ERROR_INVALID_MESSAGE     = 2,
    AXK_INTERLINK_ERROR_DIDNT_SEND          = 3
};

#define AXK_INTERLINK_FLAG_NONE         0
#define AXK_INTERLINK_FLAG_DONT_FREE    1


/*
    axk_interlink_send
    * Sends an interlink message to another processor on the system
    * 'target_cpu' is the OS-identifier for the processor you want to send the message to
    * 'in_message' is a pointer to the message structure to be sent
    * 'b_checked' will wait for the message to send, and check for an error before returning
*/
uint32_t axk_interlink_send( uint32_t target_cpu, struct axk_interlink_message_t* in_message, bool b_checked );

/*
    axk_interlink_broadcast
    * Broadcasts an Interlink message to all processors on the system
    * 'in_message' is a pointer to the message structure to be sent
    * 'b_include_self' determines if the message should also be sent to the current processor or not
    * 'b_checked' will wait for the message to send, and check for any errors before returning
*/
uint32_t axk_interlink_broadcast( struct axk_interlink_message_t* in_message, bool b_include_self, bool b_checked );

/*
    axk_interlink_set_handler
    * Sets a handler function for a specific message type
    * If an Interlink message is recieved matching this type, the callback will be invoked
    * You can also use this to clear a handler previously sent, by passing in 'NULL' for 'callback'
*/
void axk_interlink_set_handler( uint32_t type, void( *callback )( struct axk_interlink_message_t* ) );



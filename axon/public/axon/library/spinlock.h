/*==============================================================
    Axon Kernel - Spinlock Library
    2021, Zachary Berry
    axon/public/axon/library/spinlock.h
==============================================================*/

#pragma once
#include "axon/memory/atomics.h"
#include "axon/arch.h"


struct axk_spinlock_t
{
    struct axk_atomic_flag_t flag;
    uint64_t rflags;
};


void axk_spinlock_acquire( struct axk_spinlock_t* lock )
{
    // Disable interrupts and get a copy of RFLAGS
    uint64_t rflags = axk_disable_interrupts();

    // Wait to acquire the lock
    while( axk_atomic_test_and_set_flag( &( lock->flag ), MEMORY_ORDER_SEQ_CST ) )
    {
        #ifdef _X86_64_
        __asm__( "pause" );
        #endif
    }

    // Store RFLAGS in the structure
    lock->rflags = rflags;
}

void axk_spinlock_release( struct axk_spinlock_t* lock )
{
    // Read back the old RFLAGS into a local
    uint64_t rflags = lock->rflags;

    // Release the lock
    axk_atomic_clear_flag( &( lock->flag ), MEMORY_ORDER_SEQ_CST );

    // Restore previous interrupt state
    axk_restore_interrupts( rflags );
}

bool axk_spinlock_is_locked( struct axk_spinlock_t* lock )
{
    return axk_atomic_test_flag( &( lock->flag ), MEMORY_ORDER_SEQ_CST );
}

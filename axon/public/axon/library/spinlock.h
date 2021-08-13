/*==============================================================
    Axon Kernel - Spinlock Library
    2021, Zachary Berry
    axon/public/axon/library/spinlock.h
==============================================================*/

#pragma once
#include "axon/library/atomic.h"
#include "axon/kernel/kernel.h"


struct axk_spinlock_t
{
    struct axk_atomic_flag_t flag;
    uint64_t rflags;
};

/*
    axk_spinlock_init
*/
void axk_spinlock_init( struct axk_spinlock_t* lock );

/*
    axk_spinlock_acquire
*/
void axk_spinlock_acquire( struct axk_spinlock_t* lock );

/*
    axk_spinlock_release
*/
void axk_spinlock_release( struct axk_spinlock_t* lock );

/*
    axk_spinlock_is_locked
*/
bool axk_spinlock_is_locked( struct axk_spinlock_t* lock );

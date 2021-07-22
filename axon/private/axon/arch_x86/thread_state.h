/*==============================================================
    Axon Kernel - Thread State (x86)
    2021, Zachary Berry
    axon/private/axon/arch_x86/thread_state.h
==============================================================*/

#pragma once
#ifdef _X86_64_

#include <stdint.h>
#include <stdbool.h>

/*
    axk_x86_thread_state_t (x86)
    * Private Structure
    * Stores the state of a processor between time slices
*/
struct axk_x86_thread_state_t
{
    // General Registers
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    // Segment Registers
    uint16_t cs;
    uint16_t ds;
    uint16_t es;
    uint16_t fs;
    uint16_t gs;

    // Stack
    uint64_t rsp;
    uint64_t rbp;

    // Execution
    uint64_t rip;
    uint64_t rflags;

    // FX State
    uint8_t fx_state[ 512 ];
};

#endif
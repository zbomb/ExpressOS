/*==============================================================
    Axon Kernel - Config
    2021, Zachary Berry
    axon/private/axon/config.h
==============================================================*/
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define AXK_KERNEL_VA_PHYSICAL  0xFFFF800000000000
#define AXK_KERNEL_VA_HEAP      0xFFFFC00000000000
#define AXK_KERNEL_VA_SHARED    0xFFFFE00000000000
#define AXK_KERNEL_VA_IMAGE     0xFFFFFFFF80000000

#define AXK_USER_VA_IMAGE       0x100000000
#define AXK_USER_VA_SHARED      0x400000000000
#define AXK_USER_VA_STACKS      0x7F0000000000

#define AXK_KHEAP_MIN_ALLOC     0x20
#define AXK_KHEAP_ALIGN         0x10
#define AXK_KHEAP_VALIDATE      true

#define AXK_MINREQ_MEMORY       0x40000000 // 1GB

#define AXK_PAGE_SIZE               0x1000
#define AXK_KERNEL_STACK_SIZE       AXK_PAGE_SIZE * 2
#define AXK_USER_MAX_STACK_SIZE     AXK_PAGE_SIZE * 2048
#define AXK_USER_MIN_STACK_SIZE     AXK_PAGE_SIZE * 2

#define AXK_AP_INIT_PAGE        8
#define AXK_AP_INIT_ADDRESS     AXK_PAGE_SIZE * AXK_AP_INIT_PAGE

#ifdef _X86_64_
#define AXK_MAX_INTERRUPT_HANDLERS 128
#define AXK_INT_ERROR 0x30
#define AXK_INT_LOCAL_TIMER 0x31
#define AXK_INT_EXTERNAL_TIMER 0x30
#define AXK_INT_NMI 0x02
#else
#define AXK_MAX_INTERRUPT_HANDLERS 128
#define AXK_INT_ERROR 0x30
#define AXK_INT_LOCAL_TIMER 0x31
#define AXK_INT_EXTERNAL_TIMER 0x30
#define AXK_INT_NMI 0x02
#endif

typedef uint32_t AXK_PROCESS;
#define AXK_PROCESS_KERNEL 1U
#define AXK_PROCESS_INVALID 0U

typedef uint64_t AXK_PAGE_ID;
typedef uint32_t AXK_PAGE_FLAGS;
typedef uint64_t AXK_MAP_FLAGS;

#define AXK_FLAG_NONE 0x00

#define AXK_FLAG_PAGE_PREFER_HIGH       0x40000000U
#define AXK_FLAG_PAGE_CLEAR_ON_LOCK     0x20000000U
#define AXK_FLAG_PAGE_TYPE_IMAGE        0x10000000U
#define AXK_FLAG_PAGE_TYPE_HEAP         0x08000000U
#define AXK_FLAG_PAGE_TYPE_PTABLE       0x04000000U
#define AXK_FLAG_PAGE_TYPE_RAMDISK      0x02000000U
#define AXK_FLAG_PAGE_TYPE_ACPI         0x01000000U

/*
    Flags used by memory mapping functions
*/
#define AXK_FLAG_MAP_ALLOW_OVERWRITE    0x800000000000000UL

/*
    Flags used by all paging entries
*/
#define AXK_FLAG_PAGEMAP_PRESENT          0b1
#define AXK_FLAG_PAGEMAP_WRITABLE         0b10
#define AXK_FLAG_PAGEMAP_USER_ACCESS      0b100
#define AXK_FLAG_PAGEMAP_WRITE_THROUGH    0b1000
#define AXK_FLAG_PAGEMAP_DISABLE_CACHE    0b10000
#define AXK_FLAG_PAGEMAP_ACCESSED         0b100000
#define AXK_FLAG_PAGEMAP_EXEC_DISABLED    0b1000000000000000000000000000000000000000000000000000000000000000

/*
    Flags used by entries pointing to memory
*/
#define AXK_FLAG_PAGEMAP_IS_DIRTY     0b1000000
#define AXK_FLAG_PAGEMAP_HUGE_PAGE    0b10000000
#define AXK_FLAG_PAGEMAP_GLOBAL       0b100000000

#define AXK_CHECK_FLAG( _bf_, _flag_ )          ( ( _bf_ & ( _flag_ ) ) == ( _flag_ ) )
#define AXK_CHECK_ANY_FLAG( _bf_, _flags_ )     ( ( _bf_ & ( _flags_ ) ) != 0 )
#define AXK_SET_FLAG( _bf_, _flag_ )            ( _bf_ |= ( _flag_ ) )
#define AXK_CLEAR_FLAG( _bf_, _flag_ )          ( _bf_ &= ~( _flag_ ) )

/*
    Timer Driver IDs
*/
#define AXK_TIMER_ID_NONE           0
#define AXK_TIMER_ID_PIT            1
#define AXK_TIMER_ID_HPET           2
#define AXK_TIMER_ID_LAPIC          3
#define AXK_TIMER_ID_ACPI_PM        4
#define AXK_TIMER_ID_TSC            5
#define AXK_TIMER_ID_ARM_LOCAL      6
#define AXK_TIMER_ID_ARM_GENERIC    7
#define AXK_TIMER_ID_ARM_SYSTEM     8

#define AXK_DEFAULT_YEAR            2021

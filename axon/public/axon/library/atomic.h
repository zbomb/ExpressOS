/*==============================================================
    Axon Kernel - Atomic Library
    2021, Zachary Berry
    axon/public/axon/library/atomics.h
==============================================================*/

#pragma once
#include <stdint.h>
#include <stdbool.h>

enum memory_order_t
{
    MEMORY_ORDER_RELAXED = 0,
    MEMORY_ORDER_CONSUME = 1,
    MEMORY_ORDER_ACQUIRE = 2,
    MEMORY_ORDER_RELEASE = 3,
    MEMORY_ORDER_ACQ_REL = 4,
    MEMORY_ORDER_SEQ_CST = 5
};

inline int __axk_get_gcc_mem_order( enum memory_order_t order )
{
    switch( order )
    {
        case MEMORY_ORDER_SEQ_CST:
        return __ATOMIC_SEQ_CST;

        case MEMORY_ORDER_RELAXED:
        return __ATOMIC_RELAXED;

        case MEMORY_ORDER_CONSUME:
        return __ATOMIC_CONSUME;

        case MEMORY_ORDER_ACQUIRE:
        return __ATOMIC_ACQUIRE;

        case MEMORY_ORDER_RELEASE:
        return __ATOMIC_RELEASE;

        case MEMORY_ORDER_ACQ_REL:
        return __ATOMIC_ACQ_REL;

        default:
        return __ATOMIC_SEQ_CST;
    }
}

struct axk_atomic_flag_t
{
    bool val;
};

struct axk_atomic_bool_t
{
    bool val;
};

struct axk_atomic_uint32_t
{
    uint32_t val;
};

struct axk_atomic_uint64_t
{
    uint64_t val;
};

struct axk_atomic_pointer_t
{
    void* val;
};

/*
    Atomic Bool Operations
*/
inline void axk_atomic_store_bool( struct axk_atomic_bool_t* ptr, bool in, enum memory_order_t order )
{
    __atomic_store_n( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline bool axk_atomic_load_bool( struct axk_atomic_bool_t* ptr, enum memory_order_t order )
{
    return __atomic_load_n( &( ptr->val ), __axk_get_gcc_mem_order( order ) );
}

inline bool axk_atomic_exchg_bool( struct axk_atomic_bool_t* ptr, bool in, enum memory_order_t order )
{
    return __atomic_exchange_n( &( ptr->val ), &in, __axk_get_gcc_mem_order( order ) );
}

inline bool axk_atomic_cmpexchg_bool( struct axk_atomic_bool_t* ptr, bool* expected, bool desired, bool is_strong, enum memory_order_t success_order, enum memory_order_t failure_order )
{
    return __atomic_compare_exchange_n( &( ptr->val ), expected, desired, is_strong, __axk_get_gcc_mem_order( success_order ), __axk_get_gcc_mem_order( failure_order ) );
}

/*
    Atomic Flag Operations
*/

inline void axk_atomic_clear_flag( struct axk_atomic_flag_t* ptr, enum memory_order_t order )
{
    __atomic_clear( &( ptr->val ), __axk_get_gcc_mem_order( order ) );
}

inline bool axk_atomic_test_and_set_flag( struct axk_atomic_flag_t* ptr, enum memory_order_t order )
{
    return __atomic_test_and_set( (void*) &( ptr->val ), __axk_get_gcc_mem_order( order ) );
}

inline bool axk_atomic_test_flag( struct axk_atomic_flag_t* ptr, enum memory_order_t order )
{
    return __atomic_load_n( &( ptr->val ), __axk_get_gcc_mem_order( order ) );
}

inline void axk_atomic_set_flag( struct axk_atomic_flag_t* ptr, bool in, enum memory_order_t order )
{
    __atomic_store_n( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

/*
    Atomic UInt32 Operations
*/

inline void axk_atomic_store_uint32( struct axk_atomic_uint32_t* ptr, uint32_t in, enum memory_order_t order )
{
    __atomic_store_n( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline uint32_t axk_atomic_load_uint32( struct axk_atomic_uint32_t* ptr, enum memory_order_t order )
{
    return __atomic_load_n( &( ptr->val ), __axk_get_gcc_mem_order( order ) );
}

inline uint32_t axk_atomic_exchg_uint32( struct axk_atomic_uint32_t* ptr, uint32_t in, enum memory_order_t order )
{
    return __atomic_exchange_n( &( ptr->val ), &in, __axk_get_gcc_mem_order( order ) );
}

inline bool axk_atomic_cmpexchg_uint32( struct axk_atomic_uint32_t* ptr, uint32_t* expected, uint32_t desired, bool is_strong, enum memory_order_t success_order, enum memory_order_t failure_order )
{
    return __atomic_compare_exchange_n( &( ptr->val ), expected, desired, is_strong, __axk_get_gcc_mem_order( success_order ), __axk_get_gcc_mem_order( failure_order ) );
}

inline uint32_t axk_atomic_fetch_add_uint32( struct axk_atomic_uint32_t* ptr, uint32_t in, enum memory_order_t order )
{
    return __atomic_fetch_add( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline uint32_t axk_atomic_fetch_sub_uint32( struct axk_atomic_uint32_t* ptr, uint32_t in, enum memory_order_t order )
{
    return __atomic_fetch_sub( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline uint32_t axk_atomic_fetch_and_uint32( struct axk_atomic_uint32_t* ptr, uint32_t in, enum memory_order_t order )
{
    return __atomic_fetch_and( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline uint32_t axk_atomic_fetch_or_uint32( struct axk_atomic_uint32_t* ptr, uint32_t in, enum memory_order_t order )
{
    return __atomic_fetch_or( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline uint32_t axk_atomic_fetch_xor_uint32( struct axk_atomic_uint32_t* ptr, uint32_t in, enum memory_order_t order )
{
    return __atomic_fetch_xor( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

/*
    Atomic UInt64 Operations
*/

inline void axk_atomic_store_uint64( struct axk_atomic_uint64_t* ptr, uint64_t in, enum memory_order_t order )
{
    __atomic_store_n( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline uint64_t axk_atomic_load_uint64( struct axk_atomic_uint64_t* ptr, enum memory_order_t order )
{
    return __atomic_load_n( &( ptr->val ), __axk_get_gcc_mem_order( order ) );
}

inline uint64_t axk_atomic_exchg_uint64( struct axk_atomic_uint64_t* ptr, uint64_t in, enum memory_order_t order )
{
    return __atomic_exchange_n( &( ptr->val ), &in, __axk_get_gcc_mem_order( order ) );
}

inline bool axk_atomic_cmpexchg_uint64( struct axk_atomic_uint64_t* ptr, uint64_t* expected, uint64_t desired, bool is_strong, enum memory_order_t success_order, enum memory_order_t failure_order )
{
    return __atomic_compare_exchange_n( &( ptr->val ), expected, desired, is_strong, __axk_get_gcc_mem_order( success_order ), __axk_get_gcc_mem_order( failure_order ) );
}

inline uint64_t axk_atomic_fetch_add_uint64( struct axk_atomic_uint64_t* ptr, uint64_t in, enum memory_order_t order )
{
    return __atomic_fetch_add( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline uint64_t axk_atomic_fetch_sub_uint64( struct axk_atomic_uint64_t* ptr, uint64_t in, enum memory_order_t order )
{
    return __atomic_fetch_sub( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline uint64_t axk_atomic_fetch_and_uint64( struct axk_atomic_uint64_t* ptr, uint64_t in, enum memory_order_t order )
{
    return __atomic_fetch_and( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline uint64_t axk_atomic_fetch_or_uint64( struct axk_atomic_uint64_t* ptr, uint64_t in, enum memory_order_t order )
{
    return __atomic_fetch_or( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline uint64_t axk_atomic_fetch_xor_uint64( struct axk_atomic_uint64_t* ptr, uint64_t in, enum memory_order_t order )
{
    return __atomic_fetch_xor( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}


/*
    Atomic Pointer Operations
*/

inline void axk_atomic_store_pointer( struct axk_atomic_pointer_t* ptr, void* in, enum memory_order_t order )
{
    __atomic_store_n( &( ptr->val ), in, __axk_get_gcc_mem_order( order ) );
}

inline void* axk_atomic_load_pointer( struct axk_atomic_pointer_t* ptr, enum memory_order_t order )
{
    return __atomic_load_n( &( ptr->val ), __axk_get_gcc_mem_order( order ) );
}

inline void* axk_atomic_exchg_pointer( struct axk_atomic_pointer_t* ptr, void* in, enum memory_order_t order )
{
    return __atomic_exchange_n( &( ptr->val ), &in, __axk_get_gcc_mem_order( order ) );
}

inline bool axk_atomic_cmpexchg_pointer( struct axk_atomic_pointer_t* ptr, void** expected, void* desired, bool is_strong, enum memory_order_t success_order, enum memory_order_t failure_order )
{
    return __atomic_compare_exchange_n( &( ptr->val ), expected, desired, is_strong, __axk_get_gcc_mem_order( success_order ), __axk_get_gcc_mem_order( failure_order ) );
}

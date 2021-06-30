/*==============================================================
    Axon Kernel Libk - string.h
    2021, Zachary Berry
    libk/public/string.h
==============================================================*/

#pragma once
#include <stdint.h>
#include <stddef.h>

#ifndef restrict
#define restrict __restrict
#endif

void* memcpy( void* restrict dst, const void* restrict src, size_t count );
void* memmove( void* dst, const void* src, size_t count );
void* memset( void* dst, int val, size_t count );
int memcmp( const void* ptr_a, const void* ptr_b, size_t count );
int strcmp( const char* str_a, const char* str_b );
size_t strlen( const char* str );
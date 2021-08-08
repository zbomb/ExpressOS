/*==============================================================
    T-0 Bootloader - C Library Functions
    2021, Zachary Berry
    t-zero/include/libc.h
==============================================================*/

#pragma once
#include <stdint.h>
#include <stdbool.h> 
#include <stddef.h>


int memcmp( const void* ptr_a, const void* ptr_b, size_t count );

int strcmp( const char* str_a, const char* str_b );

int strcmp_u16( const uint16_t* str_a, const uint16_t* str_b );

void* memset( void* dst, int val, size_t count );

size_t strlen( const char* str );
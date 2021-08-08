/*==============================================================
    T-0 Bootloader - C Library
    2021, Zachary Berry
    t-zero/source/libc.c
==============================================================*/
 
#include "libc.h"


int memcmp( const void* ptr_a, const void* ptr_b, size_t count )
{
    if( ptr_a == NULL && ptr_b != NULL ) { return -1; }
    if( ptr_b == NULL && ptr_a != NULL ) { return 1; }
    if( ptr_a == NULL && ptr_b == NULL ) { return 0; }
    
    const uint8_t* a = ptr_a;
    const uint8_t* b = ptr_b;

    for( size_t i = 0; i < count; i++ )
    {
        if( a[ i ] < b[ i ] ) { return -1; }
        else if( a[ i ] > b[ i ] ) { return 1; }
    }

    return 0;
}


int strcmp( const char* str_a, const char* str_b )
{
    if( str_a == NULL && str_b != NULL ) { return -1; }
    if( str_b == NULL && str_a != NULL ) { return 1; }
    if( str_a == NULL && str_b == NULL ) { return 0; }

    size_t index = 0;
    while( 1 )
    {
        if( str_a[ index ] != str_b[ index ] )
        {
            return( (int)str_a[ index ] - (int)str_b[ index ] );
        }

        if( str_a[ index ] == '\0' )
        {
            return 0;
        }

        index++;
    }

    return 0; // Uncreachable
}


int strcmp_u16( const uint16_t* str_a, const uint16_t* str_b )
{
    if( str_a == NULL && str_b != NULL ) { return -1; }
    if( str_b == NULL && str_a != NULL ) { return 1; }
    if( str_a == NULL && str_b == NULL ) { return 0; }

    size_t index = 0;
    while( 1 )
    {
        if( str_a[ index ] != str_b[ index ] )
        {
            return( (int)str_a[ index ] - (int)str_b[ index ] );
        }

        if( str_a[ index ] == '\0' )
        {
            return 0;
        }

        index++;
    }

    return 0; // Uncreachable
}


void* memset( void* dst, int val, size_t count )
{
    if( dst == NULL ) { return NULL; }

    uint8_t* p = (uint8_t*)( dst );
    for( size_t i = 0; i < count; i++ )
    {
        p[ i ] = (uint8_t)( val );
    }

    return dst;
}


size_t strlen( const char* str )
{
    size_t count = 0UL;
    if( str == NULL ) { return count; }
    while( str[ count ] != '\0' ) { count++; }
    return count;
}
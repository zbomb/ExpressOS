/*==============================================================
    Axon Kernel Libk - string.c
    2021, Zachary Berry
    libk/source/string.c
==============================================================*/

#include "string.h"

int strcmp( const char* str_a, const char* str_b )
{
    size_t index = 0;
    while( 1 )
    {
        // Check if the characters differ
        if( str_a[ index ] != str_b[ index ] )
        {
            return( (int)str_a[ index ] - (int)str_b[ index ] );
        }

        // Check for nulls
        if( str_a[ index ] == '\0' )
        {
            return 0;
        }

        // Increment counter
        index++;
    }

    return 0; // Uncreachable
}
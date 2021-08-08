/*==============================================================
    T-0 Bootloader - Console Functions
    2021, Zachary Berry
    t-zero/include/console.h
==============================================================*/

#pragma once
#include "tzero.h"

/*
    tzero_console_prints16
    * Prints a UTF-16 encoded string to the console
*/
void tzero_console_prints16( CHAR16* str );

/*
    tzero_console_prints8
    * Prints a C-style string to the console (8-bit characters, ASCII)
*/
void tzero_console_prints8( const char* str );

/*
    tzero_console_clear
    * Clears all text from the console
*/
void tzero_console_clear( void );

/*
    tzero_console_printnl
    * Sets the console output position to the start of the next line
*/
void tzero_console_printnl( void );

/*
    tzero_console_set_color
    * Sets the current foreground and background text color
    * Is NOT applied to text already printed, but future text printed will have the color set
    * Colors are pre-defined (TZERO_COLOR_*)
*/
void tzero_console_set_color( uint8_t foreground, uint8_t background );

/*
    tzero_console_set_foreground
    * Sets the current foreground text color
    * Colors are pre-defined (TZERO_COLOR_*)
*/
void tzero_console_set_foreground( uint8_t color );

/*
    tzero_console_set_background
    * Sets the current background text color
    * Colors are pre-defined (TZERO_COLOR_*)
*/
void tzero_console_set_background( uint8_t color );

/*
    tzero_console_printu32
    * Prints an unisgned 32-bit number to the console
    * Prints in base-10 format
*/
void tzero_console_printu32( uint32_t num );

/*
    tzero_console_printu64
    * Prints an unsigned 64-bit number to the console
    * Prints in base-10 format
*/
void tzero_console_printu64( uint64_t num );

/*
    tzero_console_printh32
    * Prints an unsigned 32-bit number to the console in hexidecimal format
    * Optionally can include 'leading zeros', so there will always be 8 hex digits printed
*/
void tzero_console_printh32( uint32_t num, bool b_leading_zeros );

/*
    tzero_console_printh64
    * Prints an unsigned 64-bit number to the console in hexidecimal format
    * Optionally can include 'leading zeros', so there will always be 16 hex digits printed
*/
void tzero_console_printh64( uint64_t num, bool b_leading_zeros );


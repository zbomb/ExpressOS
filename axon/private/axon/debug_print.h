/*==============================================================
    Axon Kernel - Debug Printing Library
    2021, Zachary Berry
    axon/private/axon/debug_print.h
==============================================================*/

#pragma once
#include <stdint.h>
#include <stdbool.h>


enum TERMINAL_COLOR
{
    TC_BLACK           = 0,
    TC_BLUE            = 1,
    TC_GREEN           = 2,
    TC_CYAN            = 3,
    TC_RED             = 4,
    TC_MAGENTA         = 5,
    TC_BROWN           = 6,
    TC_LIGHT_GRAY      = 7,
    TC_DARK_GRAY       = 8,
    TC_LIGHT_BLUE      = 9,
    TC_LIGHT_GREEN     = 10,
    TC_LIGHT_CYAN      = 11,
    TC_LIGHT_RED       = 12,
    TC_PINK            = 13,
    TC_YELLOW          = 14,
    TC_WHITE           = 15

};

void axk_terminal_lock( void );
void axk_terminal_unlock( void );
void axk_terminal_clear( void );
void axk_terminal_clear_row( uint32_t inRow );
void axk_terminal_clear_range( uint32_t startCol, uint32_t startRow, uint32_t endCol, uint32_t endRow );
void axk_terminal_printc( char c );
void axk_terminal_prints( const char* str );
void axk_terminal_printu32( uint32_t n );
void axk_terminal_printu64( uint64_t n );
void axk_terminal_printi32( int32_t n );
void axk_terminal_printi64( int64_t n );
void axk_terminal_printh8( uint8_t i );
void axk_terminal_printh16( uint16_t i );
void axk_terminal_printh32( uint32_t i );
void axk_terminal_printh64( uint64_t i );
void axk_terminal_printh8_lz( uint8_t i, bool bLeadingZeros );
void axk_terminal_printh16_lz( uint16_t i, bool bLeadingZeros );
void axk_terminal_printh32_lz( uint32_t i, bool bLeadingZeros );
void axk_terminal_printh64_lz( uint64_t i, bool bLeadingZeros );
void axk_terminal_dumpmem( void* offset, uint32_t count );
void axk_terminal_printnl( void );
void axk_terminal_printtab( void );
void axk_terminal_set_color_overwrite( bool b );
uint32_t axk_terminal_getrow( void );
uint32_t axk_terminal_getcol( void ); 
bool axk_terminal_is_color_overwrite_enabled( void );
void axk_terminal_setbg( enum TERMINAL_COLOR c );
void axk_terminal_setfg( enum TERMINAL_COLOR c );
void axk_terminal_fill_bg_color( enum TERMINAL_COLOR c );
void axk_terminal_fill_fg_color( enum TERMINAL_COLOR c );
enum TERMINAL_COLOR axk_terminal_getbg( void );
enum TERMINAL_COLOR axk_terminal_getfg( void );
/*==============================================================
    Axon Kernel - Basic Output Support
    2021, Zachary Berry
    axon/private/axon/boot/basic_out.h
==============================================================*/

#pragma once
#include "axon/config.h"
#include "axon/gfx/psf1.h"
/*
    axk_basicout_init
    * Private Function
    * Initializes a basic output system to draw to the screen
*/
bool axk_basicout_init();

/*
    axk_basicout_test
*/
void axk_basicout_test( void );

/*
    axk_basicout_set_fg
    * Private Function
    * Sets the foreground color for text printed using the basicout system
*/
void axk_basicout_set_fg( uint8_t r, uint8_t g, uint8_t b );

/*
    axk_basicout_set_bg
    * Private Function
    * Sets the background color for text printed using the basicout system
*/
void axk_basicout_set_bg( uint8_t r, uint8_t g, uint8_t b );


/*
    axk_basicout_prints
    * Private Function
    * Prints a string using the early terminal system
    * This is for use ONLY before a proper video driver is loaded in the boot process
*/
void axk_basicout_prints( const char* str );

/*
    axk_basicout_printnl
    * Private Function
    * Prints a new line using the early terminal system
    * This is for use ONLY before a proper video driver is laoded in the boot process
*/
void axk_basicout_printnl( void );

/*
    axk_basicout_printtab
    * Private Function
    * Prints a tab character to the basic output system
*/
void axk_basicout_printtab( void );

/*
    axk_basicout_printu32
    * Private Function
    * Prints an unsigned 32-bit number using the early terminal system
    * This is for use ONLY before a proper video driver is loaded in the boot process
*/
void axk_basicout_printu32( uint32_t num );

/*
    axk_basicout_printu64
    * Private Function
    * Prints an unsigned 64-bit number using the early terminal system
    * This is for use ONLY before a proper video driver is loaded in the boot process
*/
void axk_basicout_printu64( uint64_t num );

/*
    axk_basicout_printh32
    * Private Function
    * Prints an unsigned 32-bit number in hexidecimal using the early terminal system
    * This is for use ONLY before a proper video driver is loaded in the boot process
*/
void axk_basicout_printh32( uint32_t num, bool b_leading_zeros );

/*
    axk_basicout_printh64
    * Private Function
    * Prints an unsigned 64-bit number in hexidecimal using the early terminal system
    * This is for use ONLY before a proper video driver is loaded in the boot process
*/
void axk_basicout_printh64( uint64_t num, bool b_leading_zeros );

/*
    axk_basicout_clear
    * Private Function
    * Clears the screen, resets back to print at the top left corner of the screen
*/
void axk_basicout_clear( void );

/*
    axk_basicout_lock
    * Private Function
    * Locks the basic out system so no other threads can acquire this lock unti we release it
*/
void axk_basicout_lock( void );

/*
    axk_basicout_unlock
    * Private Function
    * Unlocks the basic out system so other thrads can now acquire a lock
*/
void axk_basicout_unlock( void );
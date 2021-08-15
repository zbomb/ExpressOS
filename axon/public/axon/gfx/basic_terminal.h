/*==============================================================
    Axon Kernel - Basic Terminal Library
    2021, Zachary Berry
    axon/public/axon/gfx/basic_terminal.h
==============================================================*/

#pragma once
#include "axon/kernel/kernel.h"
#include "axon/gfx/font_psf1.h"
#include "axon/kernel/boot_params.h"

/*
    axk_basicterminal_mode_t (ENUM)
    * The two mode available to the 'basic terminal' system
    1. BASIC_TERMINAL_MODE_CONSOLE - Provides a console/terminal like system for outputting text to the user
    2. BASIC_TERMINAL_MODE_GRAPHICS - Provides a basic library for drawing shapes/text to the screen

    NOTE: When modes are switched using 'axk_basicterminal_set_mode', the screen is cleared
*/
enum axk_basicterminal_mode_t 
{
    BASIC_TERMINAL_MODE_CONSOLE     = 0,
    BASIC_TERMINAL_MODE_GRAPHICS    = 1
};


/*
    axk_basicterminal_init
    * Initializes the 'basic terminal' system, so simple text can be written to the console before full graphics
      drivers are able to be properly loaded. Also used by the panic system.
    * NOTE: By default, the mode is set to 'BASIC_TERMINAL_MODE_CONSOLE'
*/
bool axk_basicterminal_init( struct tzero_payload_parameters_t* in_params );

/*
    axk_basicterminal_get_mode
    * Gets the current terminal mode, either BASIC_TERMINAL_MODE_CONSOLE or BASIC_TERMINAL_MODE_GRPAHICS
*/
enum axk_basicterminal_mode_t axk_basicterminal_get_mode( void );

/*
    axk_basicterminal_set_mode
    * Sets the current terminal mode, either BASIC_TERMINAL_MODE_CONSOLE or BASIC_TERMINAL_MODE_GRAPHICS
    * Automatically clears the display of all existing text/graphics
*/
void axk_basicterminal_set_mode( enum axk_basicterminal_mode_t in_mode );

/*
    axk_basicterminal_set_fg
    * Private Function
    * Sets the foreground color for text printed using the basicterminal system
    * Can be used in both modes
*/
void axk_basicterminal_set_fg( uint8_t r, uint8_t g, uint8_t b );

/*
    axk_basicterminal_set_bg
    * Private Function
    * Sets the background color for text printed using the basicterminal system
    * Can be used in both modes
*/
void axk_basicterminal_set_bg( uint8_t r, uint8_t g, uint8_t b );


/*
    axk_basicterminal_get_size
    * Private Function
    * Gets the current size of the screen in pixels
    * Can be used from either mode
*/
void axk_basicterminal_get_size( uint32_t* out_width, uint32_t* out_height );


/*
    axk_basicterminal_get_text_size
    * Private Function
    * Determines what the size of the string is, if printed to the screen
    * Ignores any special formatting (\n, \t, etc..)
    * Can be used in either mode
*/
void axk_basicterminal_get_text_size( const char* str, uint32_t* out_width, uint32_t* out_height );

/*
    axk_basicterminal_get_text_size_n
    * Private Function
    * Determines the size of a string, and allows a max character count to be specified
    * * Ignores any special formatting
    * Can be used in either mode
*/
void axk_basicterminal_get_text_size_n( const char* str, size_t count, uint32_t* out_width, uint32_t* out_height );

/*
    axk_basicterminal_clear
    * Private Function
    * Clears the screen, resets back to print at the top left corner of the screen
    * Can be used in either mode
*/
void axk_basicterminal_clear( void );

/*
    axk_basicterminal_lock
    * Private Function
    * Locks the basic out system so no other threads can acquire this lock unti we release it
    * Can be used in either mode
*/
void axk_basicterminal_lock( void );

/*
    axk_basicterminal_unlock
    * Private Function
    * Unlocks the basic out system so other thrads can now acquire a lock
    * Can be used in either mode
*/
void axk_basicterminal_unlock( void );

/*
    axk_basicterminal_prints
    * Private Function
    * Prints a string using the early terminal system
    * This is for use ONLY before a proper video driver is loaded in the boot process
    * NOTE: Can only be used in 'console' mode
*/
void axk_basicterminal_prints( const char* str );

/*
    axk_basicterminal_printnl
    * Private Function
    * Prints a new line using the early terminal system
    * This is for use ONLY before a proper video driver is laoded in the boot process
    * NOTE: Can only be used in 'console' mode
*/
void axk_basicterminal_printnl( void );

/*
    axk_basicterminal_printtab
    * Private Function
    * Prints a tab character to the basic output system
    * NOTE: Can only be used in 'console' mode
*/
void axk_basicterminal_printtab( void );

/*
    axk_basicterminal_printu32
    * Private Function
    * Prints an unsigned 32-bit number using the early terminal system
    * This is for use ONLY before a proper video driver is loaded in the boot process
    * NOTE: Can only be used in 'console' mode
*/
void axk_basicterminal_printu32( uint32_t num );

/*
    axk_basicterminal_printu64
    * Private Function
    * Prints an unsigned 64-bit number using the early terminal system
    * This is for use ONLY before a proper video driver is loaded in the boot process
    * NOTE: Can only be used in 'console' mode
*/
void axk_basicterminal_printu64( uint64_t num );

/*
    axk_basicterminal_printh32
    * Private Function
    * Prints an unsigned 32-bit number in hexidecimal using the early terminal system
    * This is for use ONLY before a proper video driver is loaded in the boot process
    * NOTE: Can only be used in 'console' mode
*/
void axk_basicterminal_printh32( uint32_t num, bool b_leading_zeros );

/*
    axk_basicterminal_printh64
    * Private Function
    * Prints an unsigned 64-bit number in hexidecimal using the early terminal system
    * This is for use ONLY before a proper video driver is loaded in the boot process
    * NOTE: Can only be used in 'console' mode
*/
void axk_basicterminal_printh64( uint64_t num, bool b_leading_zeros );

/*
    axk_basicterminal_draw_text
    * Private Function
    * Draws text to the screen at the specific position, with optionally transparent background
    * The specified point (x,y) is the top left corner of the text
    * NOTE: Can only be used in 'graphics' mode
*/
void axk_basicterminal_draw_text( const char* str, uint32_t x, uint32_t y, bool b_transparent_bg );

/*
    axk_basicterminal_draw_text_n
    * Private Function
    * Draws a specified number of characters from the string
    * See 'axk_basicterminal_draw_text'
*/
void axk_basicterminal_draw_text_n( const char* str, size_t n, uint32_t x, uint32_t y, bool b_transparent_bg );

/*
    axk_basicterminal_draw_text_box
    * Private Function
    * Draws text to the screen, constrained within set bounds, optionally with transparent background
    * If the background is NOT transparent, the whole box will be filled with the background color
    * The specified point (x,y) is the top left corner of the box
    * NOTE: Can only be used in 'graphics' mode
*/
void axk_basicterminal_draw_text_box( const char* str, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool b_transparent_bg );

/*
    axk_basicterminal_draw_number
    * Private Function
    * Draws an unsigned number to the screen at the specified position, with optionally transparent background
    * The specified point (x,y) is the top left corner of the text
    * NOTE: Can only be used in 'graphics' mode
*/
void axk_basicterminal_draw_number( uint64_t num, uint32_t x, uint32_t y, bool b_transparent_bg );

/*
    axk_basicterminal_draw_hex
    * Private Function
    * Draws a hexidecimal number to the screen at the specified position, with an optionally transparent background
    * The specified point (x,y) is the top left corner of the text
    * NOTE: Can only be used in 'graphics' moe
*/
void axk_basicterminal_draw_hex( uint64_t num, bool b_leading_zeros, uint32_t x, uint32_t y, bool b_transparent_bg );

/*
    axk_basicterminal_draw_pixel
    * Private Function
    * Draws a single pixel to the screen
    * Pixel will have the current foreground color
    * NOTE: Can only be used in 'graphics' mode
*/
void axk_basicterminal_draw_pixel( uint32_t x, uint32_t y );

/*
    axk_basicterminal_draw_box
    * Private Function
    * Draws a box that is filled with the current foreground color
    * The box can optionally have an outline, set 'outline_thickness', to the number of pixels to use for
      the outline width. A value of '0' will draw no outline
    * The outline is drawn using the current background color
    * The specified point should be the top left corner of the box
    * The outline wont make the box any larger, the specified dimensions are the outline perimeter of the outline
    * NOTE: Can only be used in 'graphics' mode
*/
void axk_basicterminal_draw_box( uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t outline_width );

/*==============================================================
    T-0 Bootloader - Utility Functions
    2021, Zachary Berry
    t-zero/include/util.h
==============================================================*/

#pragma once
#include <stdint.h>
#include <stdbool.h> 
#include <stddef.h>


/*
    memcmp
    * C Library Function
*/
int memcmp( const void* ptr_a, const void* ptr_b, size_t count );

/*
    strcmp
    * C Library Function
*/
int strcmp( const char* str_a, const char* str_b );

/*
    strcmp_u16
    * Analagous to 'strcmp', but it can compare strings that use 16-bit characters
*/
int strcmp_u16( const uint16_t* str_a, const uint16_t* str_b );

/*
    memset
    * C Library Function
*/
void* memset( void* dst, int val, size_t count );

/*
    strlen
    * C Library Function
*/
size_t strlen( const char* str );

/*
    tzero_halt
    * Stops execution of the bootloader, a function that never returns
*/
__attribute__((__noreturn__)) void tzero_halt( void );

/*
    tzero_alloc
    * Allocates memory dynamically
    * All allocated data should be freed as soon as possible using 'tzero_free'
    * All bootloader data will eventually be overwritten by the payload whenever it no longer needs
      access to the data was pass along to it at boot time
*/
void* tzero_alloc( uint64_t sz );

/*
    tzero_free
    * Frees memory previously allocated by 'tzero_alloc'
*/
void tzero_free( void* ptr );

/*
    tzero_launch
    * Calls the entry point function of the payload
    * If either parameters is not needed, pass NULL (or zero) 
*/
__attribute__((__noreturn__)) void tzero_launch( uint64_t entry_point, void* param_one, void* param_two );

/*
    tzero_open_file
    * Opens a file using the EFI library
    * Pass a non-null pointer to an 'EFI_FILE_INFO' structure to 'out_info' to recieve file information
    * This function will not fail if a NULL pointer is passed to 'out_info', the file info will simply not be read
    * If a non-NULL pointer is passed to 'out_info', and the file info is not able to be read, NULL will be returned
*/
EFI_FILE* tzero_open_file( EFI_FILE* directory, CHAR16* path, EFI_FILE_INFO* out_info );


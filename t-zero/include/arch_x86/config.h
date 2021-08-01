/*==============================================================
    T-0 Bootloader - Configuration
    2021, Zachary Berry
    t-zero/include/config.h
==============================================================*/

#pragma once

/*
    Mode Selection
    * Only uncomment one mode during a build
    * Or, automate using makefile
*/
#define TZERO_MODE_KERNEL_LOADER
//#define TZERO_MODE_INSTALLER_LOADER
//#define TZERO_MODE_RESCUE_LOADER

/*
    Target Binary
*/
#define TZERO_TARGET_KERNEL     "axon.bin"
#define TZERO_TARGET_INSTALLER  "express_installer.bin"
#define TZERO_TARGET_RESCUE     "express_rescue.bin"

/*====================================================================================
    Do not edit below this line
====================================================================================*/

#if defined( TZERO_MODE_KERNEL_LOADER ) && ( defined( TZERO_MODE_INSTALLER_LOADER ) || defined( TZERO_MODE_RESCUE_LOADER ) )
_Static_assert( 0, "T-0: Attempt to enable multiple bootloader modes in a single build" );
#endif
#if defined( TZERO_MODE_INSTALLER_LOADER ) && defined( TZERO_MODE_RESCUE_LOADER )
_Static_assert( 0, "T-0: Attempt to enable multiple bootloader modes in a single build" );
#endif

#ifdef TZERO_MODE_KERNEL_LOADER
#define TZERO_TARGET_BINARY TZERO_TARGET_KERNEL
#elif defined( TZERO_MODE_INSTALLER_LOADER )
#define TZERO_TARGET_BINARY TZERO_TARGET_INSTALLER
#elif defined( TZERO_MODE_RESCUE_LOADER )
#define TZERO_TARGET_BINARY TZERO_TARGET_RESCUE
#endif

#ifndef TZERO_TARGET_BINARY
_Static_assert( 0, "T-0: Please ensure a valid build mode is selected" );
#endif

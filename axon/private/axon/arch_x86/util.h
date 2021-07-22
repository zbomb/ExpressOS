/*==============================================================
    Axon Kernel - x86 Utilities
    2021, Zachary Berry
    axon/private/axon/arch_x86/util.h
==============================================================*/

#pragma once
#include "axon/config.h"

#ifdef _X86_64_

/*
    Constants
*/
#define AXK_X86_MSR_APIC    0x1B
#define AXK_X86_MSR_TSC     0x10
#define AXK_X86_MSR_EFFER   0xC0000080
#define AXK_X86_MSR_FS_BASE 0xC0000100
#define AXK_X86_MSR_GS_BASE 0xC0000101


/*
    Structures
*/
struct axk_x86_exception_frame_t
{
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;

} __attribute__((__packed__));
/*
    Low Address         +8     +16     +24     +32     +40     +48     +56     +64     +72     +80     +88     +96    +104    +112    +120
    |---------------------------------------------------------------------------------------------------------------------------------------|
    |  ***  | orbp  |  ret  |  r11  |  r10  |   r9  |  r8   |  rdi  |  rsi  |  rdx  |  rcx  |  rbx  |  rax  |  rip  |  cs   |  rfl  |  rsp  | Stack Top
    |---------------------------------------------------------------------------------------------------------------------------------------|
               ^Current Stack Base
    * So.. in an exception handler, we should read this structure from RBP + 0x08
*/

/*
    axk_x86_cpuid
    * Private Function
    * Calls the 'cpuid' instruction
*/
void axk_x86_cpuid( uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx );

/*
    axk_x86_cpuid_s
    * Private Function
    * Checks if the leaf is supported, if not, returns false
    * Otherwise, writes the result from CPUID into the provided uint32_t's
    * If the leaf wasnt supported, 0 is written to all parameters as well
*/
bool axk_x86_cpuid_s( uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx );

/*
    axk_x86_read_msr
    * Private Function
    * Reads a model specific register
    * Returns edx:eax as a uint64_t
*/
uint64_t axk_x86_read_msr( uint32_t reg );

/*
    axk_x86_write_msr
    * Private Function
    * Writes a model-specific register
    * eax gets its value from the low 32-bits of 'value'
    * edx gets its value from the high 32-bits of 'value'
*/
void axk_x86_write_msr( uint32_t reg, uint64_t value );

/*
    axk_x86_write_gs
    * Private Function
    * Writes a value into the GS register
*/
void axk_x86_write_gs( uint64_t value );

/*
    axk_x86_read_gs
    * Private Function
    * Reads the GS address
*/
uint64_t axk_x86_read_gs( void );

/*
    axk_x86_inb
    * Private Function
    * Reads a byte from the specified port
*/
inline static uint8_t axk_x86_inb( uint16_t port ) 
{ 
    uint8_t _out_; 
    __asm__ volatile( "inb %1, %0" : "=a"( _out_ ) : "Nd"( port ) ); 
    return _out_; 
}

/*
    axk_x86_outb
    * Private Function
    * Writes a byte to the specified port
*/
inline static void axk_x86_outb( uint16_t port, uint8_t value )
{
    __asm__ volatile( "outb %0, %1" : : "a"( value ), "Nd"( port ) );
}

/*
    axk_x86_waitio
    * Private Function
    * Waits for any I/O (port) work to complete
*/
inline static void axk_x86_waitio( void )
{
    __asm__ volatile( "outb %%al, $0x80" : : "a"( 0 ) );
}

/*
    axk_x86_read_timestamp
    * Private Function
    * Reads the local processors timestamp counter
*/
inline static uint64_t axk_x86_read_timestamp( void )
{
    uint64_t rax, rdx;
    __asm__ volatile( "rdtsc" : "=a"( rax ), "=d"( rdx ) : : );
    return( ( rdx << 32 ) | rax );
}

/*
    axk_x86_read_cr4
    * Private Function
    * Reads the value in the CR4 control register
*/
inline static uint64_t axk_x86_read_cr4( void )
{
    uint64_t _out_;
    __asm__ volatile( "movq %%cr4, %0" : "=r"( _out_ ) : );
    return _out_;
}

/*
    axk_x86_write_cr4
    * Private Function
    * Writes a value into the cr4 control register
*/
inline static void axk_x86_write_cr4( uint64_t value )
{
    __asm__ volatile( "movq %0, %%cr4" : : "r"( value ) );
}

/*
    axk_x86_convert_cpu_id
    * Private Function
    * Converts the OS assigned processor identifier to an X86 interrupt identifier
    * Defined in entry point code file
*/
bool axk_x86_convert_cpu_id( uint32_t os_id, uint32_t* out_id );


#endif
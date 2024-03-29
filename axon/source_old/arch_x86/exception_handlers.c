/*==============================================================
    Axon Kernel - Exception Handlers (x86)
    2021, Zachary Berry
    axon/source/arch_x86/exception_handlers.c
==============================================================*/

#include "axon/config.h"
#include "axon/arch_x86/util.h"
#include "axon/boot/basic_out.h"
#include "axon/arch.h"
#include "axon/system/interrupts.h"


void axk_x86_handle_exception_divbyzero( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "======================> x86 - Divide By Zero Exception <======================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tFaulting instruction address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    axk_basicout_printnl();

    axk_halt();
}


void axk_x86_handle_exception_debug( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "======================> x86 - Debug Exception Raised <======================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tNext Instruction Address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    axk_basicout_printnl();
    axk_basicout_prints( "\n\tIn the future, we will allow return to execution\n" );
    
    axk_halt();
}


void axk_x86_handle_exception_nmi( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "======================> x86 - Non-Maskable Interrupt <======================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_halt();
}


void axk_x86_handle_exception_breakpoint( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "======================> x86 - Breakpoint Exception Raised <======================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tNext Instruction Address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    axk_basicout_printnl();
    
    axk_halt();
}


void axk_x86_handle_exception_overflow( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "======================> x86 - Overflow Exception Raised <======================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tFaulting instruction address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    axk_basicout_printnl();
    
    axk_halt();
}


void axk_x86_handle_exception_boundrange( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "======================> x86 - Bounds Exception Raised <======================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tFaulting instruction address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    axk_basicout_printnl();
    
    axk_halt();
}


void axk_x86_handle_exception_invalidop( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "====================> x86 - Invalid OP-Code Exception <====================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tFaulting instruction address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    axk_basicout_printnl();
    axk_basicout_prints( "\n\tTODO: Disassemble and print the instruction\n" ); // Intel has a library we can use for this
    
    axk_halt();
}


void axk_x86_handle_exception_devicenotavailable( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "===================> x86 - Device Unavailable Exception <===================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_halt();
}


void axk_x86_handle_exception_doublefault( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "====================> x86 - Double-Fault Exception Raised <====================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_halt();
}


void axk_x86_handle_exception_invalidtss( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x18UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "=====================> x86 - Invalid-TSS Exception <=====================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tFaulting instruction address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    axk_basicout_printnl();
    axk_basicout_prints( "\n\tBad Selector Index: " );
    uint32_t index = *( (uint32_t*)( _rbp + 0x10UL ) );
    axk_basicout_printh32( index, false );
    
    axk_halt();
}


void axk_x86_handle_exception_segnotpresent( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x18UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "===================> x86 - Segment Not Present Exception <===================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tFaulting instruction address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    axk_basicout_printnl();
    axk_basicout_prints( "\n\tBad Selector Index: " );
    uint32_t err_code = *( (uint32_t*)( _rbp + 0x10UL ) );
    axk_basicout_printh32( err_code, false );
    
    axk_halt();
}


void axk_x86_handle_exception_segfault( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x18UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "====================> x86 - Segment Fault Exception <====================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tFaulting instruction address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    axk_basicout_printnl();
    axk_basicout_prints( "\n\tSelector Index: " );
    uint32_t err_code = *( (uint32_t*)( _rbp + 0x10UL ) );
    axk_basicout_printh32( err_code, false );
    
    axk_halt();
}


void axk_x86_handle_exception_generalprotection( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x18UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "====================> x86 - General Protection Exception <====================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tFaulting instruction address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    axk_basicout_printnl();
    axk_basicout_prints( "\n\tSelector Index: " );
    uint32_t err_code = *( (uint32_t*)( _rbp + 0x10UL ) );
    axk_basicout_printh32( err_code, false );
    
    axk_halt();
}


void axk_x86_handle_exception_pagefault( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x18UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "====================> x86 - Page Fault Exception Raised <====================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tFaulting instruction address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    axk_basicout_printnl();
    axk_basicout_prints( "\n\tError Code: " );
    uint32_t err_code = *( (uint32_t*)( _rbp + 0x10UL ) );
    axk_basicout_printh32( err_code, false );
    axk_basicout_prints( "\tThe bad address: " );
    uint64_t cr2;
    __asm__( "movq %%cr2, %0" : "=r"( cr2 ) :: );
    axk_basicout_printh64( cr2, true );
    axk_basicout_prints( "\n\t In the future.. we will be able to return from these\n" );
    
    axk_halt();
}


void axk_x86_handle_exception_floatingpoint( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "====================> x86 - Floating-Point Exception <====================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tFaulting instruction address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    
    axk_halt();
}


void axk_x86_handle_exception_alignmentcheck( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "=====================> x86 - Alignment-Check Exception <=====================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );
    
    axk_basicout_prints( "\tFaulting instruction address: " );
    axk_basicout_printh64( ptr_frame->rip, true );
    
    axk_halt();
}


void axk_x86_handle_exception_machinecheck( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "====================> x86 - Machine-Check Exception Raised <====================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_basicout_prints( "\n\n" );

    axk_halt();
}


void axk_x86_handle_exception_virtualization( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "====================> x86 - Virtualization Exception Raised <====================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_halt();
}


void axk_x86_handle_exception_security( void )
{
    // Read interrupt stack frame
    uint64_t _rbp;
    __asm__( "movq %%rbp, %0" : "=r"( _rbp ) :: );
    struct axk_x86_exception_frame_t* ptr_frame = (struct axk_x86_exception_frame_t*)( _rbp + 0x10UL );

    axk_basicout_lock();
    axk_basicout_clear();
    axk_basicout_prints( "====================> x86 - Security Exception Raised <====================\n\n" );
    axk_basicout_prints( "\t\t\tRSP: " ); axk_basicout_printh64( ptr_frame->rsp, true ); axk_basicout_printtab();
    axk_basicout_prints( "RFLAGS: " ); axk_basicout_printh64( ptr_frame->rflags, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tCS: " ); axk_basicout_printh64( ptr_frame->cs, true ); axk_basicout_printtab();
    axk_basicout_prints( " RIP: " ); axk_basicout_printh64( ptr_frame->rip, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRAX: " ); axk_basicout_printh64( ptr_frame->rax, true ); axk_basicout_printtab();
    axk_basicout_prints( "RBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRBX: " ); axk_basicout_printh64( ptr_frame->rbx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RCX: " ); axk_basicout_printh64( ptr_frame->rcx, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDX: " ); axk_basicout_printh64( ptr_frame->rdx, true ); axk_basicout_printtab();
    axk_basicout_prints( "RSI: " ); axk_basicout_printh64( ptr_frame->rsi, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tRDI: " ); axk_basicout_printh64( ptr_frame->rdi, true ); axk_basicout_printtab();
    axk_basicout_prints( "R8: " ); axk_basicout_printh64( ptr_frame->r8, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR9: " ); axk_basicout_printh64( ptr_frame->r9, true ); axk_basicout_printtab();
    axk_basicout_prints( " R10: " ); axk_basicout_printh64( ptr_frame->r10, true ); axk_basicout_printnl();
    axk_basicout_prints( "\t\t\tR11: " ); axk_basicout_printh64( ptr_frame->r11, true );  
    axk_basicout_prints( "\tCPU ID: "); axk_basicout_printu32( axk_get_cpu_id() );
    
    axk_halt();
}


void axk_x86_handle_lapic_error( void )
{
    axk_basicout_prints( "==> Recieved Local APIC Error!  Error Code: " );
    axk_basicout_printh32( axk_interrupts_get_error(), false );
    axk_basicout_printnl();

    axk_interrupts_signal_eoi();
}


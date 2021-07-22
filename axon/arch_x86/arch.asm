;======================================================================
;   Axon Kernel - Architecture Specific Library
;   2021, Zachary Berry
;   axon/arch_x86_64/arch.asm
;======================================================================

%define AXK_VIRTUAL_OFFSET 0xFFFFFFFF80000000

global axk_disable_interrupts
global axk_restore_interrupts
global axk_enable_interrupts
global axk_halt
global axk_get_return_address
global axk_get_kernel_offset
global axk_get_kernel_size
global axk_cleanup_bootstrap

extern ax_kernel_begin
extern ax_kernel_end
extern ax_kernel_page_count
extern ax_pdt_low

section .text
bits 64

axk_disable_interrupts:

    ; Parameters:   None
    ; Returns:      Old RFLAGS (in rax)

    pushfq
    pop rax
    cli
    ret

axk_restore_interrupts:

    ; Parameters:   RFLAGS (in rdi)
    ; Returns:      None
    
    push rdi
    popfq
    ret

axk_enable_interrupts:

    ; Parameters:   None
    ; Returns:      Old RFLAGS (in rax)

    pushfq
    pop rax
    sti
    ret

axk_halt:

    ; Parameters:   None
    ; Returns:      None (Call never returns)

    .loop:
    hlt
    jmp .loop

axk_get_kernel_offset:

    ; Parameters:   None
    ; Returns       Kernel Offset (in rax)

    mov rax, ax_kernel_begin + AXK_VIRTUAL_OFFSET
    ret

axk_get_kernel_size:

    ; Parameters:   None
    ; Returns:      Kernel Image Size (in rax)

    mov rax, ax_kernel_end - AXK_VIRTUAL_OFFSET
    mov rcx, ax_kernel_begin
    sub rax, rcx
    ret

axk_cleanup_bootstrap:

    ; Parameters:   None
    ; Returns:      None

    ; Remove all the lower memory mappings we no longer need
    mov rcx, 0
    mov rax, ax_pdt_low
    .unmap_loop:

    mov byte [rax], 0
    inc rcx
    inc rax
    cmp rcx, 4096
    jl .unmap_loop

    ret



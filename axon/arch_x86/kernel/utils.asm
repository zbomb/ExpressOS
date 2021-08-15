;======================================================================
;   Axon Kernel - Utility Functions (ASM)
;   2021, Zachary Berry
;   axon/arch_x86_64/kernel/utils.asm
;======================================================================

%define AXK_VIRTUAL_OFFSET 0xFFFFFFFF80000000

global axk_interrupts_disable
global axk_interrupts_restore
global axk_interrupts_enable
global axk_halt
global axk_get_kernel_offset
global axk_get_kernel_size

extern axk_kernel_begin
extern axk_kernel_end

section .text
bits 64


axk_interrupts_disable:

    ; Parameters:   None
    ; Returns:      Old RFLAGS (in rax)

    pushfq
    pop rax
    cli
    ret

axk_interrupts_restore:

    ; Parameters:   RFLAGS (in rdi)
    ; Returns:      None
    
    push rdi
    popfq
    ret

axk_interrupts_enable:

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
    ; Returns       The virtual address of the begining of the kernel (RAX)

    mov rax, axk_kernel_begin + AXK_VIRTUAL_OFFSET
    ret

axk_get_kernel_size:

    ; Parameters:   None
    ; Returns:      The size of the kernel in bytes (RAX)

    mov rax, axk_kernel_end - AXK_VIRTUAL_OFFSET
    mov rcx, axk_kernel_begin
    sub rax, rcx
    ret
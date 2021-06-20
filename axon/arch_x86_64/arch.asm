;======================================================================
;   Axon Kernel - Architecture Library
;   2021, Zachary Berry
;   axon/arch_x86_64/arch.asm
;======================================================================

global axk_disable_interrupts
global axk_restore_interrupts
global axk_enable_interrupts

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
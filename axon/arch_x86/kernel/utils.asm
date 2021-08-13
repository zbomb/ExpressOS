;======================================================================
;   Axon Kernel - Utility Functions (ASM)
;   2021, Zachary Berry
;   axon/arch_x86_64/kernel/utils.asm
;======================================================================

global axk_interrupts_disable
global axk_interrupts_restore
global axk_interrupts_enable
global axk_halt


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
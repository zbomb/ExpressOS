;======================================================================
;   Axon Kernel - Aux Processor Main
;   2021, Zachary Berry
;   axon/arch_x86_64/bootstrap/ap_main.asm
;======================================================================

; =================== Imports/Exports ====================
global axk_ap_trampoline
global axk_ap_stack
global axk_ap_counter
global axk_ap_wait_flag

extern axk_c_main_ap
extern axk_gdt_pointer


section .data
align 8

axk_ap_stack:
    dq 0

axk_ap_counter:
    dd 0

axk_ap_wait_flag:
    db 0

section .text
bits 64
align 8

axk_ap_trampoline:

    ; Set segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Load the high address GDT pointer
    lgdt [axk_gdt_pointer]

    ; Setup our fixed kernel stack
    mov rbp, qword [axk_ap_stack]
    mov rsp, rbp

    ; Increment processor counter
    lock inc dword [axk_ap_counter]
    
    ; Wait for the flag to be set to continue
    .wait:
    pause
    cmp byte [axk_ap_wait_flag], 1
    jne .wait

    ; Now jump into the C code in the higher half
    mov rax, axk_c_main_ap
    jmp rax
    hlt
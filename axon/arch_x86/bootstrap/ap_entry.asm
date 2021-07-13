;======================================================================
;   Axon Kernel - Aux Processor Entry
;   2021, Zachary Berry
;   axon/arch_x86_64/bootstrap/ap_entry.asm
;======================================================================

; ===================== Preprocessor =====================
%define AX_HIGH_KERNEL_OFFSET       0xFFFFFFFF80000000
%define AX_FIX_ADDR( _PTR_ )        ( _PTR_ - AX_HIGH_KERNEL_OFFSET )


; ====================== Exports/Imports ====================
global axk_get_ap_code_begin
global axk_get_ap_code_size

extern ax_pml4
extern ax_gdt_pointer_low 
extern ax_ap_trampoline

; ====================== Code ======================
section .text
bits 16
align 16

ax_ap_code_begin:   ; @ 0x8000

    cli
    cld
    jmp 0x00:0x8040

align 16
ap_gdt:         ; @ 0x8010

    ;; Global Descriptor Table
    dq 0 ; Null Entry

    ; Code Segment 0x08
    dw 0xFFFF               ; Segment Limit Bits    [0,15]
    dw 0x0000               ; Base Address Bits     [0,15]
    db 0x00                 ; Base Address Bits     [16,23]
    db 0b10011010           ; Access Bits
    db 0b11001111           ; Flags & Limit Bits    [16,19]
    db 0x00                 ; Base Address Bits     [24,31]

    ; Data Segment 0x10
    dw 0xFFFF               ; Segment Limit Bits    [0,15]
    dw 0x0000               ; Base Address Bits     [0,15]
    db 0x00                 ; Base Address Bits     [16,23]
    db 0b10010010           ; Access Bits
    db 0b11001111           ; Flags & Limit Bits    [16,19]
    db 0x00                 ; Base Address Bits     [24,31]

    ; Task State Segment 0x18
    dw 0x0068               ; Segment Limit Bits    [0,15]
    dw 0x0000               ; Base Address Bits     [0,15]
    db 0x00                 ; Base Address Bits     [16,23]
    db 0b10001001           ; Access Bits
    db 0b11001111           ; Flags & Limit Bits    [16,19]
    db 0x00                 ; Base Address Bits     [24,31]

ap_gdt_pointer:     ; @ 0x8030

    dw $ - ap_gdt - 1
    dd 0x8010
    dq 0

align 64
ap_setup32:         ; @ 0x8040

    ; Load the temp GDT
    xor ax, ax              ; 0x8040
    mov ds, ax              ; 0x8043

    lgdt [0x8030]           ; 0x8045

    ; Enable protected mode
    mov eax, cr0            ; 0x804C
    or eax, 0x00000001      ; 0x804F
    mov cr0, eax            ; 0x8052

    mov ax, 0x10            ; 0x8055
    mov ds, ax              ; 0x8059
    mov ss, ax              ; 0x805B
    mov es, ax              ; 0x805D
    mov fs, ax              ; 0x805F
    mov gs, ax              ; 0x8061

    ; Long jump to 32-bit code
    jmp 0x08:0x8080         ; 0x8063
                            ; 0x806A

align 32
bits 32
ap_setup64:         ; @ 0x8080

    ; Now we need to enable paging
    ; Once we do, we can use the low identity mappings for the kernel 
    ; Then enable long mode and jump into 64-bit code
    mov eax, AX_FIX_ADDR( ax_pml4 )
    mov cr3, eax
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    mov eax, cr0
    or eax, 1 << 31
    and eax, 0x9FFFFFFF ; We want to ensure bits 30, and 29 are not set
    mov cr0, eax

    ; Now that we have paging enabled, we want to load the 'main' GDT, before jumping to x86-64 code
    ; Then, we can jump to the 64-bit trampoline code, that will get us to the C++ entry point function
    lgdt [AX_FIX_ADDR( ax_gdt_pointer_low)]
    jmp 0x08:AX_FIX_ADDR( ax_ap_trampoline )

ax_ap_code_end:


;; ============ Exported Functions ==============
bits 64
axk_get_ap_code_begin:

    mov rax, ax_ap_code_begin
    ret

axk_get_ap_code_size:

    mov rax, ax_ap_code_end
    mov rdx, ax_ap_code_begin
    sub rax, rdx
    xor rdx, rdx
    ret


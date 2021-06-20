;======================================================================
;   Axon Kernel - Bootstrap Processor Main
;   2021, Zachary Berry
;   axon/arch_x86_64/bootstrap/bsp_main.asm
;======================================================================

; =========== Preprocessor ===========
%define AX_HIGH_KERNEL_OFFSET       0xFFFFFFFF80000000
%define AX_FIX_ADDR( _PTR_ )        ( _PTR_ - AX_HIGH_KERNEL_OFFSET )

; =========== Globals/Externs ===========
global ax_x86_bsp_trampoline

extern ax_pdpt_high
extern ax_pdt_high_1
extern ax_pdt_high_2
extern ax_pml4
extern ax_gdt_pointer_high
extern ax_mb_info_pointer
extern ax_c_main_bsp

; =========== Code Section ===========
section .text
bits 64

; This is where we jump to after entering 64-bit mode, were still running in the low address space
ax_x86_bsp_trampoline:

    ; Set the segment registers
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Now we need to map the high kernel address space [0xFFFFFFFF80000000-0xFFFFFFFFFFFFFFFF]
    mov ecx, 0

    .page_map_loop:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011
    mov dword [AX_FIX_ADDR( ax_pdt_high_1 + ( ecx * 8 ))], eax
    inc ecx
    cmp ecx, 512
    jl .page_map_loop

    mov ecx, 0

    .second_page_map_loop:
    mov eax, 0x200000
    mul ecx
    add eax, 0x40000000
    or eax, 0b10000011
    mov dword [AX_FIX_ADDR( ax_pdt_high_2 + ( ecx * 8 ))], eax
    inc ecx
    cmp ecx, 512
    jl .second_page_map_loop

    mov rax, AX_FIX_ADDR( ax_pdt_high_1 )
    or rax, 0b11
    mov [qword AX_FIX_ADDR( ax_pdpt_high + 4080 )], rax

    mov rax, AX_FIX_ADDR( ax_pdt_high_2 )
    or rax, 0b11
    mov [qword AX_FIX_ADDR( ax_pdpt_high + 4088)], rax

    mov rax, AX_FIX_ADDR( ax_pdpt_high )
    or rax, 0b11
    mov [qword AX_FIX_ADDR( ax_pml4 + 4088)], rax

    ; Now, we need to jump to the 64-bit boot stub
    mov rax, ax_bsp_main
    jmp rax

; This is the first function called using the 'high kernel address space' 
ax_bsp_main:

    ; Fix the stack pointer
    mov rax, rsp
    or rax, AX_HIGH_KERNEL_OFFSET
    mov rsp, rax

    ; Reload the GST using the high pointer
    lgdt [ax_gdt_pointer_high]

    ; Fix the multiboot structure pointer
    xor rax, rax
    mov eax, dword [ax_mb_info_pointer]
    or rax, AX_HIGH_KERNEL_OFFSET
    mov qword [ax_mb_info_pointer], rax

    ; Jump to 'main' C function, should not return
    jmp ax_c_main_bsp

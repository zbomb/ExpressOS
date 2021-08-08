;======================================================================
;   Axon Kernel - Bootstrap Processor Entry 
;   2021, Zachary Berry
;   axon/arch_x86_64/bootstrap/bsp_entry.asm
;======================================================================

; =========== Preprocessor ===========
%define AXK_BSP_STACK_SIZE_PAGES    32
%define AXK_HIGH_KERNEL_OFFSET      0xFFFFFFFF80000000
%define AXK_FIX_ADDR( _PTR_ )       ( _PTR_ - AXK_HIGH_KERNEL_OFFSET )
%define AXK_TZERO_MAGIC_VALUE       0x4C4946544F464621
%define AXK_MIN_BASIC_CPUID_LEAF    4
%define AXK_MIN_EXT_CPUID_LEAF      1

; =========== Globals/Externs ===========
global axk_x86_bsp_entry
global axk_gdt_pointer
global axk_pml4
global axk_pdpt_high
global axk_pdpt_low
global axk_pdt_high_1
global axk_pdt_high_2
global axk_pdt_low
global axk_x86_get_kernel_begin
global axk_x86_get_kernel_end
global axk_x86_debug_paint_buffer

extern axk_kernel_begin
extern axk_kernel_end
extern axk_x86_c_bsp_entry

; =========== Read-Only Data ===========
section .rodata
align 16

; Global Descriptor Table
axk_gdt:

    ; Null Entry
    dq 0x00

    ; Code Segment 0x08
    dw 0xFFFF               ; Segment Limit Bits    [0,15]
    dw 0x0000               ; Base Address Bits     [0,15]
    db 0x00                 ; Base Address Bits     [16,23]
    db 0b10011010           ; Access Bits
    db 0b10101111           ; Flags & Limit Bits    [16,19]
    db 0x00                 ; Base Address Bits     [24,31]

    ; Data Segment 0x10
    dw 0xFFFF               ; Segment Limit Bits    [0,15]
    dw 0x0000               ; Base Address Bits     [0,15]
    db 0x00                 ; Base Address Bits     [16,23]
    db 0b10010010           ; Access Bits
    db 0b11001111           ; Flags & Limit Bits    [16,19]
    db 0x00                 ; Base Address Bits     [24,31]

    ; GDT Pointer
axk_gdt_pointer:

    dw $ - axk_gdt - 1
    dq axk_gdt

    ; Error Strings
_hardware_err_str:
    db "The system hardware is unsupported", 0

; ============ Normal Data ============
section .data

axk_kernel_page_count:
    dd 0x00

axk_tzero_params:
    dq 0x00

; ============ Uninitialized Data ============
section .bss
align 4096

; Page Tables
axk_pml4:
    resb 0x1000

axk_pdpt_high:
    resb 0x1000

axk_pdpt_low:
    resb 0x1000

axk_pdt_high_1:
    resb 0x1000

axk_pdt_high_2:
    resb 0x1000

axk_pdt_low:
    resb 0x1000

axk_bsp_stack_begin:
    resb AXK_BSP_STACK_SIZE_PAGES * 0x1000
axk_bsp_stack_end:


; =========== Code Section ===========
section .text
bits 64
align 8

axk_x86_get_kernel_begin:

    mov rax, axk_kernel_begin
    ret

axk_x86_get_kernel_end:

    mov rax, axk_kernel_end
    ret

check_hardware_support:

    ; Returns 1 in RAX if required features are supported, 0 if not
    ; First, were going to ensure CPUID is supported
    ; We can do this by flipping a bit in the flags register, and checking if it stays flipped
    ; TODO: We can probably assume CPUID is supported, since were booted from UEFI 64-bit enviornment anyway

    xor rax, rax
    xor rcx, rcx

    pushfq
    pop rax
    mov rcx, rax
    xor rax, 1 << 21
    push rax
    popfq
    pushfq
    pop rax
    push rcx
    popfq

    cmp eax, ecx
    je .missing_support

    ; Next up, were going to check the supported CPUID leaves, and ensure we have the required ones
    mov eax, 0
    cpuid
    cmp eax, AXK_MIN_BASIC_CPUID_LEAF
    jl .missing_support

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000000 + AXK_MIN_EXT_CPUID_LEAF
    jl .missing_support

    ; Check to ensure that MSRs are supported
    mov eax, 0x80000001
    cpuid
    cmp edx, 1 << 5
    jz .missing_support

    ; Finally, check to ensure there is a local APIC
    cmp edx, 1 << 9
    jz .missing_support

    ; Basic functionality is present
    mov rax, 1
    ret

    .missing_support:
    mov rax, 0
    ret


map_kernel_address_space:

    ; This requires two PDTs, each with 512 entries
    mov rcx, 0
    .map_begin:

    mov rax, 0x200000
    mul rcx
    or rax, 0b10000011  ; Set 'present', 'writable' and 'huge page' flags for this PDT entry
    mov r8, rax
    mov rax, 8
    mul rcx
    mov r9, AXK_FIX_ADDR( axk_pdt_high_1 )
    add rax, r9
    mov qword [rax], r8
    mov rax, 0x200000
    mov rdx, rcx
    add rdx, 512
    mul rdx
    or rax, 0b10000011  ; Set 'present', 'writable' and 'huge page' flags for this PDT entry
    mov r8, rax
    mov rax, 8
    mul rcx
    mov r9, AXK_FIX_ADDR( axk_pdt_high_2 )
    add rax, r9
    mov qword [rax], r8
    inc rcx
    cmp rcx, 512
    jb .map_begin

    ; Now we need to setup the PDPT (entries 510, 511) and the PML4 (entry 511)
    mov rax, AXK_FIX_ADDR( axk_pdt_high_1 )
    or rax, 0b11    ; Set 'present' and 'writable' flags
    mov rsi, AXK_FIX_ADDR( axk_pdpt_high ) + ( 510 * 8 )
    mov qword [rsi], rax

    mov rax, AXK_FIX_ADDR( axk_pdt_high_2 )
    or rax, 0b11    ; Set 'present' and 'writable' flags
    mov rsi, AXK_FIX_ADDR( axk_pdpt_high ) + ( 511 * 8 )
    mov qword [rsi], rax

    mov rax, AXK_FIX_ADDR( axk_pdpt_high )
    or rax, 0b11    ; Set 'present' and 'writable' flags
    mov rsi, AXK_FIX_ADDR( axk_pml4 ) + ( 511 * 8 )
    mov qword [rsi], rax

    ; Next we need to copy in the UEFI mappings, this way we can still access info passed in from the bootloader
    ; Later on, we will ditch these mappings once we get all physical memory mapped to the high address range (AXK_VA_PHYSICAL)
    mov r8, cr3
    mov r9, AXK_FIX_ADDR( axk_pml4 )
    mov rcx, 0
    .copy_loop:

    mov rax, r8
    add rax, rcx
    mov dl, byte [rax]
    mov rax, r9
    add rax, rcx
    mov byte [rax], dl

    inc rcx
    cmp rcx, 0x800
    jl .copy_loop

    ; Now we need to update the active PML4, since we still are using the one from the UEFI enviornment
    ; But, we want to store the old PML4, so we can copy in any entires in the lower half (0 to 255)
    mov rax, AXK_FIX_ADDR( axk_pml4 )
    mov cr3, rax

    ; And now, lets jump to running code at the correct virtual address
    mov rax, .begin_high_address
    jmp rax

    .begin_high_address:
    ret


axk_x86_bsp_entry:

    cli
    or rdi, rdi
    jz invalid_bootparams
    mov rax, AXK_TZERO_MAGIC_VALUE
    cmp rsi, rax
    jne invalid_bootparams
    cmp qword [rdi], rax
    jne invalid_bootparams

    ; Store the pointer to bootloader parameters so we can pass it to the entry point later on
    mov rax, AXK_FIX_ADDR( axk_tzero_params )
    mov qword [rax], rdi

    ; Setup a stack
    mov rsp, AXK_FIX_ADDR( axk_bsp_stack_end )
    mov rbp, rsp

    ; Check for support of required features
    call check_hardware_support
    or rax, rax
    jz missing_hardware_support

    ; Now, were going to map the first 2GB of physical space to the high kernel address range
    call map_kernel_address_space

    ; Finally, were going to load our updated GDT and jump to C code
    ; TODO: Implement a better GDT system, possibly per CPU? Easy to update from C code?
    lgdt [axk_gdt_pointer]

    mov rax, 0x08
    push rax
    push finish_bootstrap
    rex64 retf

invalid_bootparams:

    ;; TODO: Find a good way to display an error to the screen
    hlt

missing_hardware_support:
    
    ; We can use the callback to display an error message, should be [boot_params + 0x10]
    ; This takes two parameters, each a string, in RDI & RSI
    mov rax, qword [ AXK_FIX_ADDR( axk_tzero_params ) ]
    mov rdx, [rax]
    add rdx, 0x10
    mov rdi, _hardware_err_str
    mov rsi, 0
    call [rdx]
    hlt


finish_bootstrap:

    ; Update the segment registers
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Update the stack pointer to use the newly mapped high address range
    mov rsp, axk_bsp_stack_end
    mov rbp, rsp

    ; Lastly, were going to jump to the x86 C entry point
    mov rdi, qword [axk_tzero_params]
    call axk_x86_c_bsp_entry


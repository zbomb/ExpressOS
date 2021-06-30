;======================================================================
;   Axon Kernel - Bootstrap Processor Entry 
;   2021, Zachary Berry
;   axon/arch_x86_64/bootstrap/bsp_entry.asm
;======================================================================

; =========== Preprocessor ===========
%define AX_KERNEL_STACK_SIZE_MB     2
%define AX_MULTIBOOT_MAGIC_VALUE    0x36D76289
%define AX_HIGH_KERNEL_OFFSET       0xFFFFFFFF80000000
%define AX_FIX_ADDR( _PTR_ )        ( _PTR_ - AX_HIGH_KERNEL_OFFSET )

; =========== Globals/Externs ===========
global ax_x86_bsp_entry
global ax_gdt_pointer_high
global ax_gdt_pointer_low
global ax_pml4
global ax_pdpt_high
global ax_pdpt_low
global ax_pdt_high_1
global ax_pdt_high_2
global ax_pdt_low
global ax_mb_info_pointer

extern ax_kernel_begin
extern ax_kernel_end
extern ax_x86_bsp_trampoline

; =========== Read-Only Data ===========
section .rodata
align 16

; Global Descriptor Table
ax_gdt:

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

    ; Higher-Half GDT Pointer
ax_gdt_pointer_high:

    dw $ - ax_gdt - 1
    dq ax_gdt

    ; Lower-Half GDT Pointer
ax_gdt_pointer_low:

    dw ax_gdt_pointer_high - ax_gdt - 1
    dq AX_FIX_ADDR( ax_gdt )

; ============ Normal Data ============
section .data

ax_mb_magic_value:
    dd 0x00

ax_mb_info_pointer:
    dq 0x00

ax_kernel_page_count:
    dd 0x00

; ============ Uninitialized Data ============
section .bss
align 4096

; Page Tables
ax_pml4:
    resb 0x1000

ax_pdpt_high:
    resb 0x1000

ax_pdpt_low:
    resb 0x1000

ax_pdt_high_1:
    resb 0x1000

ax_pdt_high_2:
    resb 0x1000

ax_pdt_low:
    resb 0x1000

; Bootstrap Processor Kernel Stack
; TODO: Once memory systems are working, ditch this stack for a dynamically allocated one
ax_bsp_stack_begin:
    resb 64 * 1024
ax_bsp_stack_end:

; =========== Code Section ===========
section .text
bits 32

ax_x86_bsp_entry:

    ; Store values that are left in the registers by multiboot2
    cli
    mov dword [AX_FIX_ADDR( ax_mb_info_pointer )], ebx
    mov dword [AX_FIX_ADDR( ax_mb_magic_value )], eax
    xor eax, eax
    xor ebx, ebx

    ; Set the starting stack
    mov esp, AX_FIX_ADDR( ax_bsp_stack_end )
    mov ebp, esp

    ; Ensure we were booted from a multiboot2 compliant bootloader
    cmp dword [AX_FIX_ADDR( ax_mb_magic_value )], AX_MULTIBOOT_MAGIC_VALUE
    jne .no_multiboot

    ; Check if the CPU supports the features we need
    ; First, check to ensure CPUID instruction is supported, by flipping bit 21 of the EFLAGS register
    ; pushing it back, and popping back to check if the bit stayed flipped
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd

    cmp eax, ecx
    je .no_cpuid

    ; Check for longmode support using CPUID
    ; First ensure 0x80000001 leaf is supported
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_longmode

    ; Get the extended info leaf and check longmode bit
    mov eax, 0x80000001
    cpuid
    cmp edx, 1 << 29
    jz .no_longmode

    ; Check to ensure there is a local APIC
    cmp edx, 1 << 9
    jz .no_local_apic

    ; Check for model-specific register support
    cmp edx, 1 << 5
    jz .no_msr

    ; Next up, we need to identity map the kernel, so we can enable paging
    ; First, we need to set present and writable flags in the first entry in PML4
    mov eax, AX_FIX_ADDR( ax_pdpt_low )
    or eax, 0b11
    mov [AX_FIX_ADDR( ax_pml4 )], eax

    ; Next, do the same but to write an entry in the lower-half PDPT to the lower-half PDT
    mov eax, AX_FIX_ADDR( ax_pdt_low )
    or eax, 0b11
    mov [AX_FIX_ADDR( ax_pdpt_low )], eax

    ; We start by calculating the number of 2MB pages we need to access all of the kernel, and multiboot structure in memory
    xor edx, edx
    mov eax, AX_FIX_ADDR( ax_kernel_end )
    mov ebx, dword [AX_FIX_ADDR( ax_mb_info_pointer )]
    add ebx, dword [ebx]
    cmp eax, ebx
    jg .use_kernel_image_end
    mov eax, ebx

    .use_kernel_image_end:
    mov ecx, 0x200000
    div ecx
    or edx, edx
    jz .page_calc_no_remainder
    inc eax

    .page_calc_no_remainder:
    inc eax
    cmp eax, 512
    jg .page_calc_on_greater
    mov dword [AX_FIX_ADDR( ax_kernel_page_count )], eax
    jmp .page_calc_complete
    .page_calc_on_greater:
    mov dword [AX_FIX_ADDR( ax_kernel_page_count )], 512

    .page_calc_complete:
    mov edi, eax

    ; Now, we need to loop through the pages needed and actually set up the mapping
    mov ecx, 0
    .page_map_loop:

    mov eax, 0x200000
    mul ecx ; eax *= ecx (physical page address)
    or eax, 0b10000011 ; Set 'present' 'writable' and 'huge page' flags
    mov [AX_FIX_ADDR( ax_pdt_low ) + ( ecx * 8 ) ], eax ; Write entry to ax_pdt_low[ ecx ]
    inc ecx
    cmp ecx, edi
    jl .page_map_loop

    ; Load the page table, and enter 64-bit mode
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
    mov cr0, eax

    ; Finally, we can load the GDT and jump to the 'trampoline' code section
    lgdt [AX_FIX_ADDR( ax_gdt_pointer_low )]
    jmp 0x08:AX_FIX_ADDR( ax_x86_bsp_trampoline )

    ; ---------- Error Handlers -----------
    .no_multiboot:
    mov ax, 'MB'
    jmp .print_entry_error

    .no_cpuid:
    mov ax, 'CI'
    jmp .print_entry_error

    .no_longmode:
    mov ax, 'LM'
    jmp .print_entry_error

    .no_local_apic:
    mov ax, 'IC'
    jmp .print_entry_error

    .no_msr:
    mov ax, 'MR'
    jmp .print_entry_error

    ; TODO: Add better error message display
    .print_entry_error:
    mov dword [0xb8000], 0x0F3D0F3D     ; '=='
    mov dword [0xb8004], 0x0F3D0F3D     ; '=='
    mov dword [0xb8008], 0x0F3D0F3D     ; '=='
    mov dword [0xb800C], 0x0F3D0F3D     ; '=='
    mov dword [0xb8010], 0x0F3D0F3D     ; '=='
    mov dword [0xb8014], 0x0F3D0F3D     ; '=='
    mov dword [0xb8018], 0x0F200F3E     ; '> '
    mov dword [0xb801C], 0x0F6F0F42     ; 'Bo'
    mov dword [0xb8020], 0x0F740F6F     ; 'ot'
    mov dword [0xb8024], 0x0F460F20     ; ' F'
    mov dword [0xb8028], 0x0F690F61     ; 'ai'
    mov dword [0xb802C], 0x0F750F6C     ; 'lu'
    mov dword [0xb8030], 0x0F650F72     ; 're'
    mov dword [0xb8034], 0x0F3C0F20     ; ' <'
    mov dword [0xb8038], 0x0F3D0F3D     ; '=='
    mov dword [0xb803C], 0x0F3D0F3D     ; '=='
    mov dword [0xb8040], 0x0F3D0F3D     ; '=='
    mov dword [0xb8044], 0x0F3D0F3D     ; '=='
    mov dword [0xb8048], 0x0F3D0F3D     ; '=='
    mov dword [0xb804C], 0x0F3D0F3D     ; '=='

    ; Go to the next line, and write info about the failure
    mov dword [0xb80A8], 0x4F684F54     ; 'Th'
    mov dword [0xb80AC], 0x4F204F65     ; 'e '
    mov dword [0xb80B0], 0x4F504F43     ; 'CP'
    mov dword [0xb80B4], 0x4F204F55     ; 'U '
    mov dword [0xb80B8], 0x4F6F4F64     ; 'do'
    mov dword [0xb80BC], 0x4F734F65     ; 'es'
    mov dword [0xb80C0], 0x4F274F6E     ;'n''
    mov dword [0xb80C4], 0x4F204F74     ; 't '
    mov dword [0xb80C8], 0x4F754F73     ; 'su'
    mov dword [0xb80CC], 0x4F704F70     ; 'pp'
    mov dword [0xb80D0], 0x4F724F6F     ; 'or'
    mov dword [0xb80D4], 0x4F204F74     ; 't '

    cmp ax, 'LM'
    je .print_no_longmode

    cmp ax, 'CI'
    je .print_no_cpuid

    cmp ax, 'MB'
    je .print_no_multiboot

    ; TODO: Support the other error modes
    mov dword [0xb80D8], 0x4F6E4F55     ; 'Un'
    mov dword [0xb80DC], 0x4F6E4F6B     ; 'kn'
    mov dword [0xb80E0], 0x4F774F6F     ; 'ow'
    mov dword [0xb80E4], 0x4F214F6E     ; 'n?'
    hlt

    .print_no_longmode:
    mov dword [0xb80D8], 0x4F344F36     ; '64'
    mov dword [0xb80DC], 0x4F624F20     ; ' b'
    mov dword [0xb80E0], 0x4F744F69     ; 'it'
    mov dword [0xb80E4], 0x4F6D4F20     ; ' m'
    mov dword [0xb80E8], 0x4F644F6F     ; 'od'
    mov dword [0xb80EC], 0x4F214F65     ; 'e!'
    hlt

    .print_no_cpuid:
    mov dword [0xb80D8], 0x4F504F43     ; 'CP'
    mov dword [0xb80DC], 0x4F494F55     ; 'UI'
    mov dword [0xb80E0], 0x4F214F44     ; 'D!'
    hlt

    .print_no_multiboot:
    mov dword [0xb80D8], 0x4F754F4D     ; 'Mu'
    mov dword [0xb80DC], 0x4F744F6C     ; 'lt'
    mov dword [0xb80E0], 0x4F624F69     ; 'ib'
    mov dword [0xb80E4], 0x4F6F4F6F     ; 'oo'
    mov dword [0xb80E8], 0x4F214F74     ; 't!'
    hlt

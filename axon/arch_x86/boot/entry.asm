;======================================================================
;   Axon Kernel - Bootstrap Entry 
;   2021, Zachary Berry
;   axon/arch_x86_64/bootstrap/entry.asm
;======================================================================

; =========== Preprocessor ===========
%define AXK_BSP_STACK_SIZE_PAGES    32
%define AXK_HIGH_KERNEL_OFFSET      0xFFFFFFFF80000000
%define AXK_FIX_ADDR( _PTR_ )       ( _PTR_ - AXK_HIGH_KERNEL_OFFSET )
%define AXK_TZERO_MAGIC_VALUE       0x4C4946544F464621
%define AXK_TZERO_ARCH_X86          0x80000000
%define AXK_MIN_BASIC_CPUID_LEAF    4
%define AXK_MIN_EXT_CPUID_LEAF      0x80000001

%define GENERIC_PARAMS_MAGIC_OFFSET         0
%define GENERIC_PARAMS_SUCCESS_FN_OFFSET    8
%define GENERIC_PARAMS_FAILURE_FN_OFFSET    16
%define X86_PARAMS_MAGIC_OFFSET             0
%define X86_PARAMS_ARCH_CODE_OFFSET         8

;================================== Globals and Externs ===================================
global axk_pml4
global axk_pdpt_511
global axk_x86_entry
extern axk_x86_main

;================================== Read-Only Data ===================================
section .rodata
align 8

default_error_string:
    db "No error information was provided! The installation might be corrupted", 0

invalid_x86_parameters_string:
    db "The x86 specific parameters were invalid! The installation might be corrupted", 0

missing_cpuid_string:
    db "The processor does not support CPUID, which is required! Ensure the system meets processor requirments", 0

missing_cpuid_basic_leaves_string:
    db "The processor does not have the level of basic CPUID support that is required! Ensure the system meets processor requirments", 0

missing_cpuid_ext_leaves_string:
    db "The processor does not have the level of extended CPUID support that is required! Ensure the system meets processor requirments", 0

missing_msr_string:
    db "The processor does not support model-specific registers, which is required! Ensure the system meets processor requirments", 0

missing_lapic_string:
    db "The system does not have LAPIC support, which is required! Ensure the system meets processor requirments", 0

missing_misc_string:
    db "The system does not have required feature support! Ensure the system meets processor requirments", 0

;================================== Data ===================================
section .data
align 8

callback_error:
    dq 0x00

parameters_generic_pointer:
    dq 0x00

parameters_x86_pointer:
    dq 0x00

;================================== Uninitialized Data ===================================
section .bss
align 8

bsp_stack_top:
    resb AXK_BSP_STACK_SIZE_PAGES * 0x1000
bsp_stack_bottom:

align 4096
axk_pml4:
    resb 0x1000

axk_pdpt_511:
    resb 0x1000

axk_kernel_pdt_0:
    resb 0x1000

axk_kernel_pdt_1:
    resb 0x1000

;================================== Code ===================================
section .text
bits 64
align 8

critical_error_without_callback:

    ; If we jump to here, there was an error, and we dont have a way to actually display this error to the user
    ; This can happen when the 'error callback' is NULL, the generic parameters are NULL or invalid
    ; For now, were just going to 'hlt' because there isnt much of an alternative
    hlt

invoke_error:

    ; Parameters: rdi : Error string (const char*) [Requires physical address]
    ; Control is never returned from this function

    or rdi, rdi
    jnz .get_callback
    mov rdi, AXK_FIX_ADDR( default_error_string )
    .get_callback:
    mov r8, qword AXK_FIX_ADDR( callback_error )
    mov rax, [r8]
    or rax, rax
    jz critical_error_without_callback
    jmp rax

check_hardware_support:

    ; Parameters: none
    ; Returns: none
    ; If there is something missing, we will invoke the error and not return to the caller

    ; Check for CPUID support (RFLAGS bit 21, check if it can be modified)
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
    cmp rax, rcx
    je .missing_cpuid

    ; Check the max available CPUID leaf, and extended CPUID leaf
    xor rax, rax
    cpuid
    cmp eax, AXK_MIN_BASIC_CPUID_LEAF
    jb .missing_basic_leaves

    mov eax, 0x80000000
    cpuid
    cmp eax, AXK_MIN_EXT_CPUID_LEAF
    jb .missing_ext_leaves

    ; Ensure model-specific registers are supported
    mov eax, 0x01
    cpuid
    test edx, 1 << 5
    jz .missing_msr

    ; Ensure there is a local-apic controller
    test edx, 1 << 9
    jz .missing_lapic

    ; Ensure sysenter/sysexit are supported
    test edx, 1 << 1
    jz .missing_misc

    ; Ensure TSC is supported
    test edx, 1 << 4
    jz .missing_misc

    ; Everything we tested was supported!
    ret

    .missing_cpuid:
    mov rdi, AXK_FIX_ADDR( missing_cpuid_string )
    jmp invoke_error
    .missing_basic_leaves:
    mov rdi, AXK_FIX_ADDR( missing_cpuid_basic_leaves_string )
    jmp invoke_error
    .missing_ext_leaves:
    mov rdi, AXK_FIX_ADDR( missing_cpuid_ext_leaves_string )
    jmp invoke_error
    .missing_msr:
    mov rdi, AXK_FIX_ADDR( missing_msr_string )
    jmp invoke_error
    .missing_lapic:
    mov rdi, AXK_FIX_ADDR( missing_lapic_string )
    jmp invoke_error
    .missing_misc:
    mov rdi, AXK_FIX_ADDR( missing_misc_string )
    jmp invoke_error


create_memory_map:

    ; Parameters: none
    ; Returns: none
    ; We are going to take the UEFI mappings and copy them, then add the high address space mappings for the kernel
    
    xor rcx, rcx
    .map_loop:

    mov rax, 0x200000
    mul rcx
    or rax, 0b10000011  ; Set 'present', 'writable' and 'huge page' flags for this PDT entry
    mov r8, rax
    mov rax, 8
    mul rcx
    mov r9, AXK_FIX_ADDR( axk_kernel_pdt_0 )
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
    mov r9, AXK_FIX_ADDR( axk_kernel_pdt_1 )
    add rax, r9
    mov qword [rax], r8
    inc rcx
    cmp rcx, 512
    jb .map_loop

    ; Now we need to setup the PDPT (entries 510, 511) and the PML4 (entry 511)
    mov rax, AXK_FIX_ADDR( axk_kernel_pdt_0 )
    or rax, 0b11    ; Set 'present' and 'writable' flags
    mov rsi, AXK_FIX_ADDR( axk_pdpt_511 ) + ( 510 * 8 )
    mov qword [rsi], rax

    mov rax, AXK_FIX_ADDR( axk_kernel_pdt_1 )
    or rax, 0b11    ; Set 'present' and 'writable' flags
    mov rsi, AXK_FIX_ADDR( axk_pdpt_511 ) + ( 511 * 8 )
    mov qword [rsi], rax

    mov rax, AXK_FIX_ADDR( axk_pdpt_511 )
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

    ; Load page mappings and start execuiting code at the correct address using a long jump
    mov rax, AXK_FIX_ADDR( axk_pml4 )
    mov cr3, rax
    mov rax, .begin_high_address
    jmp rax
    .begin_high_address:

    ret


axk_x86_entry:

    cli

    ; First, we need to validate the 'generic' parameters held in rdi
    or rdi, rdi
    jz critical_error_without_callback
    mov rax, AXK_TZERO_MAGIC_VALUE
    cmp qword [rdi + GENERIC_PARAMS_MAGIC_OFFSET], rax
    jne critical_error_without_callback
    cmp qword [rdi + GENERIC_PARAMS_SUCCESS_FN_OFFSET], 0
    jz critical_error_without_callback
    cmp qword [rdi + GENERIC_PARAMS_FAILURE_FN_OFFSET], 0
    jz critical_error_without_callback

    ; In the generic parameter list, were given a callback to use when theres an error, so store this pointer
    mov rax, qword [rdi + GENERIC_PARAMS_FAILURE_FN_OFFSET]
    mov r8, AXK_FIX_ADDR( callback_error )
    mov qword [r8], rax

    ; Now, lets validate the arch-specific parameter list
    or rsi, rsi
    jz .invalid_x86_parameters
    mov qword rax, AXK_TZERO_MAGIC_VALUE
    cmp qword [rsi + X86_PARAMS_MAGIC_OFFSET], rax
    jne .invalid_x86_parameters
    mov dword eax, AXK_TZERO_ARCH_X86
    cmp dword [rsi + X86_PARAMS_ARCH_CODE_OFFSET], eax
    jne .invalid_x86_parameters

    ; Store the two parameter list pointers for later access
    mov qword rax, AXK_FIX_ADDR( parameters_generic_pointer )
    mov qword [rax], rdi
    mov qword rax, AXK_FIX_ADDR( parameters_x86_pointer )
    mov qword [rax], rsi

    ; Setup a new stack, were still using the UEFI stack
    mov rsp, AXK_FIX_ADDR( bsp_stack_bottom )
    mov rbp, rsp

    ; Check for required hardware support, will jump to error handling code if something is missing and not return
    call check_hardware_support

    ; Now, we need to create our own memory mappings, since were still using the UEFI mappings
    call create_memory_map

    ; Fix the stack pointer to use the higher address
    mov rsp, bsp_stack_bottom
    mov rbp, rsp

    ; Finally, starting executing C code!
    mov rax, axk_x86_main
    mov rdi, qword [parameters_generic_pointer]
    mov rsi, qword [parameters_x86_pointer]
    call rax

    .invalid_x86_parameters:
    mov rdi, AXK_FIX_ADDR( invalid_x86_parameters_string )
    jmp invoke_error



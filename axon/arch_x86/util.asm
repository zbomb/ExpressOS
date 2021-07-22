;======================================================================
;   Axon Kernel - x86 Utility Functions
;   2021, Zachary Berry
;   axon/arch_x86_64/util.asm
;======================================================================

global axk_x86_cpuid
global axk_x86_read_msr
global axk_x86_write_msr
global axk_x86_cpuid_s
global axk_x86_write_gs
global axk_x86_read_gs

section .data
align 8

max_cpuid:
dd 0xFFFFFFFF

max_cpuid_ext:
dd 0xFFFFFFFF

section .text
bits 64


axk_x86_cpuid:

    ; Parameters:
    ; 'EAX Input/Output' => EDI, 'EBX Input/Output' => ESI,
    ; 'ECX Input/Output' => EDX, 'EDX Input/Output' => ECX

    ; Preserve RBX
    push rbx

    ; Save the addresses passed in, so we can move the results back into these addresses after cpuid is called
    mov r8, rdi
    mov r9, rsi
    mov r10, rdx
    mov r11, rcx

    ; Now, we need to get the starting values for each register, and move them into the proper place
    mov dword eax, [r8]
    mov dword ebx, [r9]
    mov dword ecx, [r10]
    mov dword edx, [r11]

    ; Call CPUID instruction
    cpuid

    ; Now, move the resulting registers back to the memory addresses we were supplied with
    mov dword [r8], eax
    mov dword [r9], ebx
    mov dword [r10], ecx
    mov dword [r11], edx

    ; Restore RBX
    pop rbx
    ret


axk_x86_cpuid_s:

    ; Parameters:
    ; 'EAX Input/Output' => EDI, 'EBX Input/Output' => ESI,
    ; 'ECX Input/Output' => EDX, 'EDX Input/Output' => ECX
    ;
    ; Returns:
    ; True if supported, false if not => EAX

    push rbx

    ; Save addresses passed in, so we can write outputs to them later
    mov r8, rdi     ; &eax
    mov r9, rsi     ; &ebx
    mov r10, rdx    ; &ecx
    mov r11, rcx    ; &edx

    ; Check if we need to load values into 'max_cpuid' and 'max_cpuid_ext'
    cmp dword [max_cpuid], 0xFFFFFFFF
    jne .check_support

    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    cpuid

    mov dword [max_cpuid], eax

    mov eax, 0x80000000
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    cpuid

    mov dword [max_cpuid_ext], eax

    .check_support:

    ; Check if this leaf is in the extended range, or the lower range
    mov eax, dword [r8]
    cmp eax, 0x80000000
    jae .check_ext_support

    ; Check lower range leaf
    cmp eax, dword [max_cpuid]
    ja .not_supported
    jmp .read_cpuid

    ; Check extended leaf
    .check_ext_support:
    cmp eax, dword [max_cpuid_ext]
    ja .not_supported

    ; Leaf is available, lets read it
    .read_cpuid:
    mov ebx, dword [r9]
    mov ecx, dword [r10]
    mov edx, dword [r11]

    cpuid

    ; Check if the leaf is not supported even though its in the available range
    or eax, eax
    jnz .is_supported
    or ebx, ebx
    jnz .is_supported
    or ecx, ecx
    jnz .is_supported
    or edx, edx
    jnz .is_supported

    .not_supported:
    mov dword [r8], 0
    mov dword [r9], 0
    mov dword [r10], 0
    mov dword [r11], 0
    mov rax, 0
    pop rbx
    ret

    .is_supported:
    mov dword [r8], eax
    mov dword [r9], ebx
    mov dword [r10], ecx
    mov dword [r11], edx
    mov rax, 1
    pop rbx
    ret

axk_x86_read_msr:

    ; Parameters:
    ; edi = 'register number'
    ; 
    ; Returns
    ; edx:eax
    
    mov ecx, edi
    rdmsr

    shl rdx, 32
    or rax, rdx

    ret

axk_x86_write_msr:

    ; Parameters:
    ; edi = 'register number', rsi = 'value' (edx:eax)
    ;
    ; Returns: none

    mov ecx, edi
    mov eax, esi
    shr rsi, 32
    mov edx, esi

    wrmsr
    ret

axk_x86_write_gs:

    ; Parameters: Value (rdi)
    ; Returns: None

    mov ecx, 0xc0000102
    mov eax, edi
    shr rdi, 32
    mov edx, edi
    wrmsr
    swapgs
    ret

axk_x86_read_gs:

    ; Parameters: None
    ; Returns: GS Address (rax)

    mov rax, qword [gs:0]
    ret
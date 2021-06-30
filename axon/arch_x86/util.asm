;======================================================================
;   Axon Kernel - x86 Utility Functions
;   2021, Zachary Berry
;   axon/arch_x86_64/util.asm
;======================================================================

global axk_x86_cpuid
global axk_x86_read_msr
global axk_x86_write_msr

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
;==============================================================
;    Axon Kernel Libk - string.asm
;    2021, Zachary Berry
;    libk/arch_x86_64/string.asm
;==============================================================


global memset
global memmove
global memcmp
global strlen
global memcpy

section .text
bits 64

;; void* memcpy( void* dst, void* src, size_t count )
;; dst = rdi, src = rsi, count = rdx
;; return to rax, should return the dest addr
memcpy:


    ; Setup stack
    push rbp
    mov rbp, rsp

    ; Push the input memory address, so when we exit we can pop it to rax
    push rdi

    ; Check for nulls, and zero count
    or rdi, rdi
    jz .invalid_params

    or rsi, rsi
    jz .invalid_params

    cmp rdx, 0
    je .invalid_params

    ; Loop through memory, and copy
    mov rcx, 0
    .loop_begin:

    mov r8b, [rsi]
    mov [rdi], r8b

    inc rcx
    inc rsi
    inc rdi

    cmp rcx, rdx
    jge .loop_exit
    jmp .loop_begin
    
    .loop_exit:
    pop rax
    pop rbp
    ret

    .invalid_params:
    pop rax
    xor rax, rax
    pop rbp
    ret

;; void* memset( void* addr, uint8_t val, size_t count )
;; addr = rdi, val = sil, count = rdx
memset:

    ; Set stack frame
    push rbp
    mov rbp, rsp

    ; Calculate the 'past end' address for the target memory range in rdx
    push rdi
    add rdx, rdi

    ; Loop through each address in the range, and copy the 8-bit into it, rdi is counter
    .loop_begin:

    cmp rdi, rdx
    jge .loop_end

    ; Write value into memory address
    mov byte [rdi], sil

    inc rdi
    jmp .loop_begin
    
    .loop_end:

    ; Pop the starting address into the return register, and restore stack frame
    pop rax
    pop rbp
    ret

;; void* memmove( void* dst, const void* src, size_t count );
;; dst = rdi, src = rsi, count = rdx
;; ret to rax
memmove:

    ; The difference between memcpy is, we properly handle overlapping memory buffers
    ; To do this, we detect which comes first in memory, and we either copy in reverse or normal order depending
    ; Setup stack
    push rbp
    mov rbp, rsp

    ; Push destination to stack, so we can pop and return it
    push rdi

    ; Check for nulls and zero count
    or rdi, rdi
    jz .func_exit
    or rsi, rsi
    jz .func_exit
    or rdx, rdx
    jz .func_exit

    ; Determine order to copy in
    ; If the destination is later than source, we copy in reverse (rdi > rsi)
    ; If the destination is before the source, we copy forwards (rdi < rsi)
    cmp rdi, rsi
    jg .do_reverse_copy
    
    ; Do a forward copy
    mov rcx, 0

    .fwdloop_begin:

    ; Store copy of the byte in r8b then copy to dest
    mov r8b, [rsi]
    mov [rdi], r8b

    ; Incremement everything
    inc rsi
    inc rdi
    inc rcx

    ; Check for end of loop
    cmp rcx, rdx
    jge .func_exit
    jmp .fwdloop_begin

    .do_reverse_copy:

    mov rcx, rdx
    add rsi, rcx
    add rdi, rcx

    .rvloop_begin:

    ; Decrement addresses and counter
    dec rcx
    dec rsi
    dec rdi

    ; Store copy of the byte in r8b and then copy to dest
    mov r8b, [rsi]
    mov [rdi], r8b

    ; Check for end of loop
    or rcx, rcx
    jz .func_exit
    jmp .rvloop_begin

    .func_exit:
    pop rax
    pop rbp
    ret


    ; Setup stack
    push rbp
    mov rbp, rsp

    ; Push the input memory address, so when we exit we can pop it to rax
    push rdi

    ; Check for nulls, and zero count
    or rdi, rdi
    jz .invalid_params

    or rsi, rsi
    jz .invalid_params

    cmp rdx, 0
    je .invalid_params

    ; Loop through memory, and copy
    mov rcx, 0
    .loop_begin:

    mov r8b, [rsi]
    mov [rdi], r8b

    inc rcx
    inc rsi
    inc rdi

    cmp rcx, rdx
    jge .loop_exit
    jmp .loop_begin
    
    .loop_exit:
    pop rax
    pop rbp
    ret

    .invalid_params:
    pop rax
    xor rax, rax
    pop rbp
    ret

;; int memcmp( const void* ptr_a, const void* ptr_b, size_t count );
;; ptr_a = rdi, ptr_b = rsi, count = rdx
;; ret to eax
memcmp:

    ; Setup stack
    push rbp
    mov rbp, rsp

    xor rax, rax

    ; Check for a count of zero
    or rdx, rdx
    jnz .valid_count

    pop rbp
    ret

    .valid_count:

    ; Check for null pointers, although this isnt part of the standard..
    or rdi, rdi
    jz .a_null
    or rsi, rsi
    jz .b_null ; If we goto 'b_null', we know a is NOT null
    jmp .func_main

    .a_null: ;;;;;;; A NULL

    ; Check if both are null
    or rsi, rsi
    jz .both_null

    ; a is null, b is not, so lets return > 1
    mov eax, 0x7FFFFFFF
    pop rbp
    ret

    .b_null: ;;;;;;; B NULL

    ; Return < 1
    mov eax, 0x80000000
    pop rbp
    ret

    .both_null: ;;;;;;;;; BOTH NULL 

    ; Return 0
    pop rbp
    ret

    .func_main: ;;;;;; Neither Null

    ; So, lets loop until we find a difference, if the byte pointed to by ptr_a is less, we return < 1, and vise versa
    ; If we hit the end of the range, and found no difference, we return 0
    mov rcx, 0
    mov r8d, 0
    .loop_begin:

    mov byte r8b, [rdi]

    ; ptr_a = r8l, ptr_b = rsi
    cmp r8b, [rsi]
    jne .found_diff

    ; Check if we hit the end of the range
    inc rcx
    inc rdi
    inc rsi

    cmp rcx, rdx
    jge .func_exit
    jmp .loop_begin

    .found_diff:
    xor rcx, rcx
    mov byte cl, [rsi]
    sub byte r8b, cl
    movsx eax, r8b

    .func_exit:
    pop rbp
    ret

;; size_t strlen( const char* str );
;; str = rdi
;; ret to rax
strlen:

    ; Setup stack
    push rbp
    mov rbp, rsp

    ; Check for null ptr
    mov rax, 0
    or rdi, rdi
    jnz .loop_begin
    
    ; Invalid ptr!
    pop rbp
    ret

    .loop_begin:
    cmp byte [rdi], 0x00
    je .loop_exit

    inc rdi
    inc rax
    jmp .loop_begin

    .loop_exit:
    pop rbp
    ret

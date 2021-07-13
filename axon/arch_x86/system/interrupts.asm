;======================================================================
;   Axon Kernel - x86 Interrupts (ASM)
;   2021, Zachary Berry
;   axon/arch_x86_64/system/interrupts.asm
;======================================================================

; Exported Functions
global axk_x86_load_idt
global axk_x86_load_idt_aux
global axk_x86_test_idt

; Imported Functions
extern axk_x86_handle_exception_divbyzero
extern axk_x86_handle_exception_debug
extern axk_x86_handle_exception_nmi
extern axk_x86_handle_exception_breakpoint
extern axk_x86_handle_exception_overflow
extern axk_x86_handle_exception_boundrange
extern axk_x86_handle_exception_invalidop
extern axk_x86_handle_exception_devicenotavailable
extern axk_x86_handle_exception_doublefault
extern axk_x86_handle_exception_invalidtss
extern axk_x86_handle_exception_segnotpresent
extern axk_x86_handle_exception_segfault
extern axk_x86_handle_exception_generalprotection
extern axk_x86_handle_exception_pagefault
extern axk_x86_handle_exception_floatingpoint
extern axk_x86_handle_exception_alignmentcheck
extern axk_x86_handle_exception_machinecheck
extern axk_x86_handle_exception_virtualization
extern axk_x86_handle_exception_security

extern axk_interrupts_invoke
extern axk_invoke_external_timer
extern axk_invoke_local_timer
extern axk_x86_handle_lapic_error


; Macros
%macro _write_interrupt 2
    mov rdi, %1
    mov sil, %2
    mov dl, 0x8E
    call _write_idt_entry
%endmacro

%macro _write_trap 2
    mov rdi, %1
    mov sil, %2
    mov dl, 0x8F
    call _write_idt_entry
%endmacro

%macro _save_registers 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
%endmacro

%macro _restore_registers 0
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

%macro _call_static_func 1
    _save_registers
    call %1
    _restore_registers
    iretq
%endmacro

%macro _invoke_handler 1
    _save_registers
    mov dil, %1
    call axk_interrupts_invoke
    _restore_registers
    iretq
%endmacro

%macro _ignore_interrupt 0
    iretq
%endmacro

; ================ Uninitialized Memory ===================
section .bss
align 16

axk_x86_idt:
    resb 0x1000

; =================== Data Section ====================
section .data
align 16

axk_x86_idt_info:
    dw 0x0FFF
    dq axk_x86_idt

; ====================== Code Section ======================
section .text
bits 64
align 8

axk_x86_test_idt:
    mov rax, qword [0xFFFFFFFFFFFFFF]
    add rax, 100
    ret

_write_idt_entry:

    ; Parameters:
    ;   Handler Address [rdi]
    ;   Interrupt Vector [sil]
    ;   Type Code [dl]
    ;
    ; Returns: None

    mov r8b, dl
    xor rax, rax
    mov al, sil
    mov rbx, 0x10
    mul rbx
    add rax, axk_x86_idt

    mov word [rax], di
    mov word [rax + 2], 0x0008
    mov byte [rax + 4], 0x00
    mov byte [rax + 5], r8b
    
    shr rdi, 16
    mov word [rax + 6], di
    shr rdi, 16
    mov dword [rax + 8], edi
    mov dword [rax + 12], 0x00000000
    ret


axk_x86_load_idt:

    ; Parameters:   None
    ; Returns:      None

    _write_interrupt  _isr_0,      0x00
    _write_trap       _isr_1,      0x01
    _write_interrupt  _isr_2,      0x02
    _write_trap       _isr_3,      0x03
    _write_trap       _isr_4,      0x04
    _write_interrupt  _isr_5,      0x05
    _write_interrupt  _isr_6,      0x06
    _write_interrupt  _isr_7,      0x07
    _write_interrupt  _isr_8,      0x08
    _write_interrupt  _isr_9,      0x09
    _write_interrupt  _isr_10,     0x0A
    _write_interrupt  _isr_11,     0x0B
    _write_interrupt  _isr_12,     0x0C
    _write_interrupt  _isr_13,     0x0D
    _write_interrupt  _isr_14,     0x0E
    _write_interrupt  _isr_15,     0x0F
    _write_interrupt  _isr_16,     0x10
    _write_interrupt  _isr_17,     0x11
    _write_interrupt  _isr_18,     0x12
    _write_interrupt  _isr_19,     0x13
    _write_interrupt  _isr_20,     0x14
    _write_interrupt  _isr_21,     0x15
    _write_interrupt  _isr_22,     0x16
    _write_interrupt  _isr_23,     0x17
    _write_interrupt  _isr_24,     0x18
    _write_interrupt  _isr_25,     0x19
    _write_interrupt  _isr_26,     0x1A
    _write_interrupt  _isr_27,     0x1B
    _write_interrupt  _isr_28,     0x1C
    _write_interrupt  _isr_29,     0x1D
    _write_interrupt  _isr_30,     0x1E
    _write_interrupt  _isr_31,     0x1F
    _write_interrupt  _isr_32,     0x20
    _write_interrupt  _isr_33,     0x21
    _write_interrupt  _isr_34,     0x22
    _write_interrupt  _isr_35,     0x23
    _write_interrupt  _isr_36,     0x24
    _write_interrupt  _isr_37,     0x25
    _write_interrupt  _isr_38,     0x26
    _write_interrupt  _isr_39,     0x27
    _write_interrupt  _isr_40,     0x28
    _write_interrupt  _isr_41,     0x29
    _write_interrupt  _isr_42,     0x2A
    _write_interrupt  _isr_43,     0x2B
    _write_interrupt  _isr_44,     0x2C
    _write_interrupt  _isr_45,     0x2D
    _write_interrupt  _isr_46,     0x2E
    _write_interrupt  _isr_47,     0x2F
    _write_interrupt  _isr_48,     0x30
    _write_interrupt  _isr_49,     0x31
    _write_interrupt  _isr_50,     0x32
    _write_interrupt  _isr_51,     0x33
    _write_interrupt  _isr_52,     0x34
    _write_interrupt  _isr_53,     0x35
    _write_interrupt  _isr_54,     0x36
    _write_interrupt  _isr_55,     0x37
    _write_interrupt  _isr_56,     0x38
    _write_interrupt  _isr_57,     0x39
    _write_interrupt  _isr_58,     0x3A
    _write_interrupt  _isr_59,     0x3B
    _write_interrupt  _isr_60,     0x3C
    _write_interrupt  _isr_61,     0x3D
    _write_interrupt  _isr_62,     0x3E
    _write_interrupt  _isr_63,     0x3F
    _write_interrupt  _isr_64,     0x40
    _write_interrupt  _isr_65,     0x41
    _write_interrupt  _isr_66,     0x42
    _write_interrupt  _isr_67,     0x43
    _write_interrupt  _isr_68,     0x44
    _write_interrupt  _isr_69,     0x45
    _write_interrupt  _isr_70,     0x46
    _write_interrupt  _isr_71,     0x47
    _write_interrupt  _isr_72,     0x48
    _write_interrupt  _isr_73,     0x49
    _write_interrupt  _isr_74,     0x4A
    _write_interrupt  _isr_75,     0x4B
    _write_interrupt  _isr_76,     0x4C
    _write_interrupt  _isr_77,     0x4D
    _write_interrupt  _isr_78,     0x4E
    _write_interrupt  _isr_79,     0x4F
    _write_interrupt  _isr_80,     0x50
    _write_interrupt  _isr_81,     0x51
    _write_interrupt  _isr_82,     0x52
    _write_interrupt  _isr_83,     0x53
    _write_interrupt  _isr_84,     0x54
    _write_interrupt  _isr_85,     0x55
    _write_interrupt  _isr_86,     0x56
    _write_interrupt  _isr_87,     0x57
    _write_interrupt  _isr_88,     0x58
    _write_interrupt  _isr_89,     0x59
    _write_interrupt  _isr_90,     0x5A
    _write_interrupt  _isr_91,     0x5B
    _write_interrupt  _isr_92,     0x5C
    _write_interrupt  _isr_93,     0x5D
    _write_interrupt  _isr_94,     0x5E
    _write_interrupt  _isr_95,     0x5F
    _write_interrupt  _isr_96,     0x60
    _write_interrupt  _isr_97,     0x61
    _write_interrupt  _isr_98,     0x62
    _write_interrupt  _isr_99,     0x63
    _write_interrupt  _isr_100,    0x64
    _write_interrupt  _isr_101,    0x65
    _write_interrupt  _isr_102,    0x66
    _write_interrupt  _isr_103,    0x67
    _write_interrupt  _isr_104,    0x68
    _write_interrupt  _isr_105,    0x69
    _write_interrupt  _isr_106,    0x6A
    _write_interrupt  _isr_107,    0x6B
    _write_interrupt  _isr_108,    0x6C
    _write_interrupt  _isr_109,    0x6D
    _write_interrupt  _isr_110,    0x6E
    _write_interrupt  _isr_111,    0x6F
    _write_interrupt  _isr_112,    0x70
    _write_interrupt  _isr_113,    0x71
    _write_interrupt  _isr_114,    0x72
    _write_interrupt  _isr_115,    0x73
    _write_interrupt  _isr_116,    0x74
    _write_interrupt  _isr_117,    0x75
    _write_interrupt  _isr_118,    0x76
    _write_interrupt  _isr_119,    0x77
    _write_interrupt  _isr_120,    0x78
    _write_interrupt  _isr_121,    0x79
    _write_interrupt  _isr_122,    0x7A
    _write_interrupt  _isr_123,    0x7B
    _write_interrupt  _isr_124,    0x7C
    _write_interrupt  _isr_125,    0x7D
    _write_interrupt  _isr_126,    0x7E
    _write_interrupt  _isr_127,    0x7F
    _write_interrupt  _isr_128,    0x80
    _write_interrupt  _isr_129,    0x81
    _write_interrupt  _isr_130,    0x82
    _write_interrupt  _isr_131,    0x83
    _write_interrupt  _isr_132,    0x84
    _write_interrupt  _isr_133,    0x85
    _write_interrupt  _isr_134,    0x86
    _write_interrupt  _isr_135,    0x87
    _write_interrupt  _isr_136,    0x88
    _write_interrupt  _isr_137,    0x89
    _write_interrupt  _isr_138,    0x8A
    _write_interrupt  _isr_139,    0x8B
    _write_interrupt  _isr_140,    0x8C
    _write_interrupt  _isr_141,    0x8D
    _write_interrupt  _isr_142,    0x8E
    _write_interrupt  _isr_143,    0x8F
    _write_interrupt  _isr_144,    0x90
    _write_interrupt  _isr_145,    0x91
    _write_interrupt  _isr_146,    0x92
    _write_interrupt  _isr_147,    0x93
    _write_interrupt  _isr_148,    0x94
    _write_interrupt  _isr_149,    0x95
    _write_interrupt  _isr_150,    0x96
    _write_interrupt  _isr_151,    0x97
    _write_interrupt  _isr_152,    0x98
    _write_interrupt  _isr_153,    0x99
    _write_interrupt  _isr_154,    0x9A
    _write_interrupt  _isr_155,    0x9B
    _write_interrupt  _isr_156,    0x9C
    _write_interrupt  _isr_157,    0x9D
    _write_interrupt  _isr_158,    0x9E
    _write_interrupt  _isr_159,    0x9F
    _write_interrupt  _isr_160,    0xA0
    _write_interrupt  _isr_161,    0xA1
    _write_interrupt  _isr_162,    0xA2
    _write_interrupt  _isr_163,    0xA3
    _write_interrupt  _isr_164,    0xA4
    _write_interrupt  _isr_165,    0xA5
    _write_interrupt  _isr_166,    0xA6
    _write_interrupt  _isr_167,    0xA7
    _write_interrupt  _isr_168,    0xA8
    _write_interrupt  _isr_169,    0xA9
    _write_interrupt  _isr_170,    0xAA
    _write_interrupt  _isr_171,    0xAB
    _write_interrupt  _isr_172,    0xAC
    _write_interrupt  _isr_173,    0xAD
    _write_interrupt  _isr_174,    0xAE
    _write_interrupt  _isr_175,    0xAF
    _write_interrupt  _isr_176,    0xB0
    _write_interrupt  _isr_177,    0xB1
    _write_interrupt  _isr_178,    0xB2
    _write_interrupt  _isr_179,    0xB3
    _write_interrupt  _isr_180,    0xB4
    _write_interrupt  _isr_181,    0xB5
    _write_interrupt  _isr_182,    0xB6
    _write_interrupt  _isr_183,    0xB7
    _write_interrupt  _isr_184,    0xB8
    _write_interrupt  _isr_185,    0xB9
    _write_interrupt  _isr_186,    0xBA
    _write_interrupt  _isr_187,    0xBB
    _write_interrupt  _isr_188,    0xBC
    _write_interrupt  _isr_189,    0xBD
    _write_interrupt  _isr_190,    0xBE
    _write_interrupt  _isr_191,    0xBF
    _write_interrupt  _isr_192,    0xC0
    _write_interrupt  _isr_193,    0xC1
    _write_interrupt  _isr_194,    0xC2
    _write_interrupt  _isr_195,    0xC3
    _write_interrupt  _isr_196,    0xC4
    _write_interrupt  _isr_197,    0xC5
    _write_interrupt  _isr_198,    0xC6
    _write_interrupt  _isr_199,    0xC7
    _write_interrupt  _isr_200,    0xC8
    _write_interrupt  _isr_201,    0xC9
    _write_interrupt  _isr_202,    0xCA
    _write_interrupt  _isr_203,    0xCB
    _write_interrupt  _isr_204,    0xCC
    _write_interrupt  _isr_205,    0xCD
    _write_interrupt  _isr_206,    0xCE
    _write_interrupt  _isr_207,    0xCF
    _write_interrupt  _isr_208,    0xD0
    _write_interrupt  _isr_209,    0xD1
    _write_interrupt  _isr_210,    0xD2
    _write_interrupt  _isr_211,    0xD3
    _write_interrupt  _isr_212,    0xD4
    _write_interrupt  _isr_213,    0xD5
    _write_interrupt  _isr_214,    0xD6
    _write_interrupt  _isr_215,    0xD7
    _write_interrupt  _isr_216,    0xD8
    _write_interrupt  _isr_217,    0xD9
    _write_interrupt  _isr_218,    0xDA
    _write_interrupt  _isr_219,    0xDB
    _write_interrupt  _isr_220,    0xDC
    _write_interrupt  _isr_221,    0xDD
    _write_interrupt  _isr_222,    0xDE
    _write_interrupt  _isr_223,    0xDF
    _write_interrupt  _isr_224,    0xE0
    _write_interrupt  _isr_225,    0xE1
    _write_interrupt  _isr_226,    0xE2
    _write_interrupt  _isr_227,    0xE3
    _write_interrupt  _isr_228,    0xE4
    _write_interrupt  _isr_229,    0xE5
    _write_interrupt  _isr_230,    0xE6
    _write_interrupt  _isr_231,    0xE7
    _write_interrupt  _isr_232,    0xE8
    _write_interrupt  _isr_233,    0xE9
    _write_interrupt  _isr_234,    0xEA
    _write_interrupt  _isr_235,    0xEB
    _write_interrupt  _isr_236,    0xEC
    _write_interrupt  _isr_237,    0xED
    _write_interrupt  _isr_238,    0xEE
    _write_interrupt  _isr_239,    0xEF
    _write_interrupt  _isr_240,    0xF0
    _write_interrupt  _isr_241,    0xF1
    _write_interrupt  _isr_242,    0xF2
    _write_interrupt  _isr_243,    0xF3
    _write_interrupt  _isr_244,    0xF4
    _write_interrupt  _isr_245,    0xF5
    _write_interrupt  _isr_246,    0xF6
    _write_interrupt  _isr_247,    0xF7
    _write_interrupt  _isr_248,    0xF8
    _write_interrupt  _isr_249,    0xF9
    _write_interrupt  _isr_250,    0xFA
    _write_interrupt  _isr_251,    0xFB
    _write_interrupt  _isr_252,    0xFC
    _write_interrupt  _isr_253,    0xFD
    _write_interrupt  _isr_254,    0xFE
    _write_interrupt  _isr_255,    0xFF

    ; Fallthrough intentional
axk_x86_load_idt_aux:

    lidt [axk_x86_idt_info]
    sti
    ret


; ======================== ISR Entries =========================

_isr_0:
    _call_static_func axk_x86_handle_exception_divbyzero
_isr_1:
    _call_static_func axk_x86_handle_exception_debug
_isr_2:
    _call_static_func axk_x86_handle_exception_nmi
_isr_3:
    _call_static_func axk_x86_handle_exception_breakpoint
_isr_4:
    _call_static_func axk_x86_handle_exception_overflow
_isr_5:
   _call_static_func axk_x86_handle_exception_boundrange
_isr_6:
   _call_static_func axk_x86_handle_exception_invalidop
_isr_7:
   _call_static_func axk_x86_handle_exception_devicenotavailable
_isr_8:
   _call_static_func axk_x86_handle_exception_doublefault
_isr_9:
    _ignore_interrupt
_isr_10:
   _call_static_func axk_x86_handle_exception_invalidtss
_isr_11:
   _call_static_func axk_x86_handle_exception_segnotpresent
_isr_12:
   _call_static_func axk_x86_handle_exception_segfault
_isr_13:
   _call_static_func axk_x86_handle_exception_generalprotection
_isr_14:
   _call_static_func axk_x86_handle_exception_pagefault
_isr_15:
    _ignore_interrupt
_isr_16:
    _call_static_func axk_x86_handle_exception_floatingpoint
_isr_17:
   _call_static_func axk_x86_handle_exception_alignmentcheck
_isr_18:
   _call_static_func axk_x86_handle_exception_machinecheck
_isr_19:
   _call_static_func axk_x86_handle_exception_floatingpoint
_isr_20:
   _call_static_func axk_x86_handle_exception_virtualization
_isr_21:
_isr_22:
_isr_23:
_isr_24:
_isr_25:
_isr_26:
_isr_27:
_isr_28:
_isr_29:
    _ignore_interrupt
_isr_30:
    _call_static_func axk_x86_handle_exception_security
_isr_31:
_isr_32:
_isr_33:
_isr_34:
_isr_35:
_isr_36:
_isr_37:
_isr_38:
_isr_39:
_isr_40:
_isr_41:
_isr_42:
_isr_43:
_isr_44:
_isr_45:
_isr_46:
_isr_47:
    _ignore_interrupt
_isr_48:
    _call_static_func axk_invoke_external_timer
_isr_49:
    _call_static_func axk_invoke_local_timer
_isr_50:
_isr_51:
_isr_52:
_isr_53:
_isr_54:
_isr_55:
_isr_56:
_isr_57:
_isr_58:
_isr_59:
_isr_60:
_isr_61:
_isr_62:
_isr_63:
    _ignore_interrupt
_isr_64:
    _invoke_handler 0
_isr_65:
    _invoke_handler 1
_isr_66:
    _invoke_handler 2
_isr_67:
    _invoke_handler 3
_isr_68:
    _invoke_handler 4
_isr_69:
    _invoke_handler 5
_isr_70:
    _invoke_handler 6
_isr_71:
    _invoke_handler 7
_isr_72:
    _invoke_handler 8
_isr_73:
    _invoke_handler 9
_isr_74:
    _invoke_handler 10
_isr_75:
    _invoke_handler 11
_isr_76:
    _invoke_handler 12
_isr_77:
    _invoke_handler 13
_isr_78:
    _invoke_handler 14
_isr_79:
    _invoke_handler 15
_isr_80:
    _invoke_handler 16
_isr_81:
    _invoke_handler 17
_isr_82:
    _invoke_handler 18
_isr_83:
    _invoke_handler 19
_isr_84:
    _invoke_handler 20
_isr_85:
    _invoke_handler 21
_isr_86:
    _invoke_handler 22
_isr_87:
    _invoke_handler 23
_isr_88:
    _invoke_handler 24
_isr_89:
    _invoke_handler 25
_isr_90:
    _invoke_handler 26
_isr_91:
    _invoke_handler 27
_isr_92:
    _invoke_handler 28
_isr_93:
    _invoke_handler 29
_isr_94:
    _invoke_handler 30
_isr_95:
    _invoke_handler 31
_isr_96:
    _invoke_handler 32
_isr_97:
    _invoke_handler 33
_isr_98:
    _invoke_handler 34
_isr_99:
    _invoke_handler 35
_isr_100:
    _invoke_handler 36
_isr_101:
    _invoke_handler 37
_isr_102:
    _invoke_handler 38
_isr_103:
    _invoke_handler 39
_isr_104:
    _invoke_handler 40
_isr_105:
    _invoke_handler 41
_isr_106:
    _invoke_handler 42
_isr_107:
    _invoke_handler 43
_isr_108:
    _invoke_handler 44
_isr_109:
    _invoke_handler 45
_isr_110:
    _invoke_handler 46
_isr_111:
    _invoke_handler 47
_isr_112:
    _invoke_handler 48
_isr_113:
    _invoke_handler 49
_isr_114:
    _invoke_handler 50
_isr_115:
    _invoke_handler 51
_isr_116:
    _invoke_handler 52
_isr_117:
    _invoke_handler 53
_isr_118:
    _invoke_handler 54
_isr_119:
    _invoke_handler 55
_isr_120:
    _invoke_handler 56
_isr_121:
    _invoke_handler 57
_isr_122:
    _invoke_handler 58
_isr_123:
    _invoke_handler 59
_isr_124:
    _invoke_handler 60
_isr_125:
    _invoke_handler 61
_isr_126:
    _invoke_handler 62
_isr_127:
    _invoke_handler 63
_isr_128:
    _invoke_handler 64
_isr_129:
    _invoke_handler 65
_isr_130:
    _invoke_handler 66
_isr_131:
    _invoke_handler 67
_isr_132:
    _invoke_handler 68
_isr_133:
    _invoke_handler 69
_isr_134:
    _invoke_handler 70
_isr_135:
    _invoke_handler 71
_isr_136:
    _invoke_handler 72
_isr_137:
    _invoke_handler 73
_isr_138:
    _invoke_handler 74
_isr_139:
    _invoke_handler 75
_isr_140:
    _invoke_handler 76
_isr_141:
    _invoke_handler 77
_isr_142:
    _invoke_handler 78
_isr_143:
    _invoke_handler 79
_isr_144:
    _invoke_handler 80
_isr_145:
    _invoke_handler 81
_isr_146:
    _invoke_handler 82
_isr_147:
    _invoke_handler 83
_isr_148:
    _invoke_handler 84
_isr_149:
    _invoke_handler 85
_isr_150:
    _invoke_handler 86
_isr_151:
    _invoke_handler 87
_isr_152:
    _invoke_handler 88
_isr_153:
    _invoke_handler 89
_isr_154:
    _invoke_handler 90
_isr_155:
    _invoke_handler 91
_isr_156:
    _invoke_handler 92
_isr_157:
    _invoke_handler 93
_isr_158:
    _invoke_handler 94
_isr_159:
    _invoke_handler 95
_isr_160:
    _invoke_handler 96
_isr_161:
    _invoke_handler 97
_isr_162:
    _invoke_handler 98
_isr_163:
    _invoke_handler 99
_isr_164:
    _invoke_handler 100
_isr_165:
    _invoke_handler 101
_isr_166:
    _invoke_handler 102
_isr_167:
    _invoke_handler 103
_isr_168:
    _invoke_handler 104
_isr_169:
    _invoke_handler 105
_isr_170:
    _invoke_handler 106
_isr_171:
    _invoke_handler 107
_isr_172:
    _invoke_handler 108
_isr_173:
    _invoke_handler 109
_isr_174:
    _invoke_handler 110
_isr_175:
    _invoke_handler 111
_isr_176:
    _invoke_handler 112
_isr_177:
    _invoke_handler 113
_isr_178:
    _invoke_handler 114
_isr_179:
    _invoke_handler 115
_isr_180:
    _invoke_handler 116
_isr_181:
    _invoke_handler 117
_isr_182:
    _invoke_handler 118
_isr_183:
    _invoke_handler 119
_isr_184:
    _invoke_handler 120
_isr_185:
    _invoke_handler 121
_isr_186:
    _invoke_handler 122
_isr_187:
    _invoke_handler 123
_isr_188:
    _invoke_handler 124
_isr_189:
    _invoke_handler 125
_isr_190:
    _invoke_handler 126
_isr_191:
    _invoke_handler 127
_isr_192:
_isr_193:
_isr_194:
_isr_195:
_isr_196:
_isr_197:
_isr_198:
_isr_199:
_isr_200:
_isr_201:
_isr_202:
_isr_203:
_isr_204:
_isr_205:
_isr_206:
_isr_207:
_isr_208:
_isr_209:
_isr_210:
_isr_211:
_isr_212:
_isr_213:
_isr_214:
_isr_215:
_isr_216:
_isr_217:
_isr_218:
_isr_219:
_isr_220:
_isr_221:
_isr_222:
_isr_223:
_isr_224:
_isr_225:
_isr_226:
_isr_227:
_isr_228:
_isr_229:
_isr_230:
_isr_231:
_isr_232:
_isr_233:
_isr_234:
_isr_235:
_isr_236:
_isr_237:
_isr_238:
_isr_239:
_isr_240:
_isr_241:
_isr_242:
_isr_243:
_isr_244:
_isr_245:
_isr_246:
_isr_247:
_isr_248:
_isr_249:
_isr_250:
_isr_251:
_isr_252:
_isr_253:
_isr_254:
_isr_255:
    _ignore_interrupt
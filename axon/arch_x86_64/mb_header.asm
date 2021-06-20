;======================================================================
;   Axon Kernel - Multiboot Header
;   2021, Zachary Berry
;   axon/arch_x86_64/mb_header.asm
;======================================================================

section .ax_multiboot_header
align 8

header_begin:

    ; Header
    dd 0xE85250D6 ; Magic value
    dd 0x00000000 ; Target architecture (Protected Mode i386)
    dd header_end - header_begin ; Length
    dd 0x100000000 - ( 0xE85250D6 + 0x00000000 + ( header_end - header_begin ) ) ; Checksum

    ;; TODO: Any info to pass to multiboot bootloader here?

    ;  End tag
    align 8
    dw 0x0000
    dw 0x0000
    dd 0x00000008

header_end:
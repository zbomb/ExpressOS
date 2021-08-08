/*==============================================================
    T-0 Bootloader - ELF Structures
    2021, Zachary Berry
    t-zero/include/elf.h
==============================================================*/

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
    Typedefs
*/
typedef uint16_t    elf_half_t;
typedef uint32_t    elf_word_t;
typedef int32_t     elf_sword_t;
typedef uint64_t    elf_xword_t;
typedef int64_t     elf_sxword_t;
typedef uint64_t    elf_addr_t;
typedef uint64_t    elf_off_t;
typedef uint16_t    self_section_t;
typedef elf_half_t  elf_versym_t;


/*
    Structures
*/
struct elf_ehdr_t
{
    uint8_t     ident[ 16 ];    // Magic number and other info
    elf_half_t  type;           // Object file type
    elf_half_t  machine;        // Architecture
    elf_word_t  version;        // Object file version
    elf_addr_t  entry;          // Entry point virtual address
    elf_off_t   phoff;          // Program header table file offset
    elf_off_t   shoff;          // Section header table file offset
    elf_word_t  flags;          // Processor-specific flags
    elf_half_t  ehsize;         // ELF header size in bytes
    elf_half_t  phentsize;      // Program header table entry size
    elf_half_t  phnum;          // Program header table entry count
    elf_half_t  shentsize;      // Section header table entry size
    elf_half_t  shnum;          // Section header table entry count
    elf_half_t  shstrndx;       // Section header string table index
};


struct elf_phdr_t
{
    elf_word_t      type;       // Segment Type
    elf_word_t      flags;      // Segment Flags
    elf_off_t       offset;     // Segment File Offset
    elf_addr_t      vaddr;      // Segment Virtual Address
    elf_addr_t      paddr;      // Segment Physical Address
    elf_xword_t     filesz;     // Segment Size In File
    elf_xword_t     memsz;      // Segment Size In Memory
    elf_xword_t     align;      // Segment Alignment
};  

/*
    Constants
*/
#define ELF_MAGIC_0     0x7F            // Magic Value Constants
#define ELF_MAGIC_1     'E'
#define ELF_MAGIC_2     'L'
#define ELF_MAGIC_3     'F'
#define ELF_MAGIC       "\177ELF"
#define ELF_MAGIC_SZ    0x04

#define EI_CLASS        0x04            // File class byte index
#define ELF_CLASS_NONE  0x00
#define ELF_CLASS_32    0x01
#define ELF_CLASS_64    0x02
#define ELF_CLASS_NUM   0x03

#define EI_DATA         0x05            // Data encoding byte index
#define ELF_DATA_NONE   0x00
#define ELF_DATA_2LSB   0x01            // 2's compilment, little endian
#define ELF_DATA_2MSB   0x02            // 2's compilment, big endian
#define ELF_DATA_NUM    0x03

#define EI_VERSION      0x06            // File version byte index

#define EI_OSABI                0x07    // OS ABI identifier index
#define ELF_OSABI_NONE          0x00
#define ELF_OSABI_SYSV          0x00
#define ELF_OSABI_HPUX          0x01
#define ELF_OSABI_NETBSD        0x02
#define ELF_OSABI_GNU           0x03
#define ELF_OSABI_LINUX         0x03
#define ELF_OSABI_SOLARIS       0x06
#define ELF_OSABI_AIX           0x07
#define ELF_OSABI_IRIX          0x08
#define ELF_OSABI_FREEBSD       0x09
#define ELF_OSABI_TRU64         0x0A
#define ELF_OSABI_MODESTO       0x0B
#define ELF_OSABI_OPENBSD       0x0C
#define ELF_OSABI_ARM_AEABI     0x40
#define ELF_OSABI_ARM           0x61
#define ELF_OSABI_STANDALONE    0xFF

#define EI_ABIVERSION   0x08            // OS ABI Version
#define EI_PAD          0x09            // Byte index of padding bytes

// Object File Types (efi_header_t.type)

#define ET_NONE     0x00
#define ET_REL      0x01        // Relocatable File
#define ET_EXEC     0x02        // Executable File
#define ET_DYN      0x03        // Shared Object File
#define ET_CORE     0x04        // Core File
#define ET_NUM      0x05        // Number of defined types
#define ET_LOOS     0xFE00      // OS-specific range start
#define ET_HIOS     0xFEFF      // OS-specific range end
#define ET_LOPROC   0xFF00      // Processor-specific range start
#define EI_HIPROC   0xFFFF      // Processor-specific range end

#define EM_NONE     0x00
#define EM_X86_64   0x3E
#define EM_AARCH64  0xB7
#define EM_NUM      0xF8

#define EV_NONE     0x00
#define EV_CURRENT  0x01
#define EV_NUM      0x02

#define PT_NULL             0x00            // Unused segment
#define PT_LOAD             0x01            // Loadable program segment
#define PT_DYNAMIC          0x02            // Dynamic linking information
#define PT_INTERP           0x03            // Program interpretor
#define PT_NOTE             0x04            // Aux information
#define PT_SHLIB            0x05            // RSVD
#define PT_PHDR             0x06            // Entry for header table
#define PT_TLS              0x07            // Thread-local-storage segment
#define PT_NUM              0x08            
#define PT_LOOS             0x60000000      // Start of OS-Specific
#define PT_GNU_EH_FRAME     0x6474E550      // GCC .eh_frame_hdr segment
#define PT_GNU_STACK        0x6474E551      // Indicates stack executability
#define PT_GNU_RELRO        0x6474E552	    // Read-only after relocation
#define PT_HIOS             0x6FFFFFFF      // End of OS-Specific 
#define PT_LOPROC           0x70000000      // Start of processor specific
#define PT_HIPROC           0x7FFFFFFF      // End of processor specific

#define PF_X            ( 1 << 0 )      // Segment is executable
#define PF_W            ( 1 << 1 )      // Segment is writable
#define PF_R            ( 1 << 2 )      // Segment is readable
#define PF_MASKOS       0x0ff00000      // OS-Specific mask
#define PF_MASKPROC     0xF0000000      // Processor-Specific mask
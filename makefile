##########################################################################
# 	ExpressOS / Axon Kernel
#	Project Makfile
#	2021, Zachary Berry
##########################################################################

################################################# Source & Build Paths #################################################

AXON_INCLUDE_PATH_PUBLIC = axon/public/
AXON_INCLUDE_PATH_PRIVATE = axon/private/
AXON_SOURCE_PATH = axon/source/

LIBC_INCLUDE_PATH_PUBLIC = libc/public/
LIBC_INCLUDE_PATH_PRIVATE = libc/private/
LIBC_SOURCE_PATH = libc/source/

LIBK_INCLUDE_PATH_PUBLIC = libk/public/
LIBK_INCLUDE_PATH_PRIVATE = libk/private/
LIBK_SOURCE_PATH = libk/source/

# TODO: Add ExpressOS Paths!

AXON_BUILD_PATH = axon/build/
LIBC_BUILD_PATH = libc/build/
LIBK_BUILD_PATH = libk/build/

################################################# C/C++ Compilation Parameters #################################################

C_CC = /usr/local/cross/bin/x86_64-elf-gcc
CPP_CC = /usr/local/cross/bin/x86_64-elf-g++

LIBC_CPARAMS ?= -O2 -g -std=c17
LIBK_CPARAMS ?= -O2 -g -std=c17
AXON_CPARAMS ?= -O2 -g -std=c17

LIBC_CPARAMS += -c -I $(LIBC_INCLUDE_PATH_PRIVATE) -I $(LIBC_INCLUDE_PATH_PUBLIC) -nostdlib -D _X86_64_ -lgcc
LIBK_CPARAMS += -c -I $(LIBK_INCLUDE_PATH_PRIVATE) -I $(LIBK_INCLUDE_PATH_PUBLIC) -I $(AXON_INCLUDE_PATH_PRIVATE) -I $(AXON_INCLUDE_PATH_PUBLIC) -ffreestanding -nostdlib -D _X86_64_ -mno-red-zone -mcmodel=kernel
AXON_CPARAMS += -c -I $(AXON_INCLUDE_PATH_PRIVATE) -I $(AXON_INCLUDE_PATH_PUBLIC) -I $(LIBK_INCLUDE_PATH_PRIVATE) -I $(LIBK_INCLUDE_PATH_PUBLIC) -ffreestanding -nostdlib -lgcc -D _X86_64_ -mno-red-zone -mcmodel=kernel

################################################# x86_64 Specific Paths #################################################

AXON_X86_64_PATH = axon/arch_x86_64/
AXON_X86_64_BUILD_PATH = axon/build/x86_64/

LIBC_X86_64_PATH = libc/arch_x86_64/
LIBC_X86_64_BUILD_PATH = libc/build/x86_64/

LIBK_X86_64_PATH = libk/arch_x86_64/
LIBK_X86_64_BUILD_PATH = libk/build/x86_64/

################################################# x86_64 Compilation Parameters #################################################

ASM_CC = nasm

AXON_ASMPARAMS ?=
AXON_ASMPARAMS += -f elf64

LIBC_ASMPARAMS ?=
LIBC_ASMPARAMS += -f elf64

LIBK_ASMPARAMS ?= 
LIBK_ASMPARAMS += -f elf64

################################################# (x86_64) Linker Parameters #################################################

LINKER = x86_64-elf-ld
ARCHIVER = ar

AXON_LPARAMS ?= -n
AXON_LPARAMS +=
AXON_LINKFILE = axon/arch_x86_64/linker.ld

LIBC_ARCPARAMS ?= 
LIBC_ARCPARAMS := rcs $(LIBC_ARCPARAMS)

LIBK_ARCPARAMS ?= 
LIBK_ARCPARAMS := rcs $(LIBK_ARCPARAMS)

AXON_OUTPUT_DIR = bin/x86_64/axon/
LIBC_OUTPUT_DIR = bin/x86_64/libc/
LIBK_OUTPUT_DIR = bin/x86_64/libk/

################################################# End Of Parameters #################################################

################################################# Finding Source Files #################################################

LIBC_SOURCE_C := $(shell find $(LIBC_SOURCE_PATH) -type f -name "*.c")
LIBK_SOURCE_C := $(shell find $(LIBK_SOURCE_PATH) -type f -name "*.c")
AXON_SOURCE_C := $(shell find $(AXON_SOURCE_PATH) -type f -name "*.c")

LIBC_SOURCE_X86_64 := $(shell find $(LIBC_X86_64_PATH) -name "*.asm")
LIBK_SOURCE_X86_64 := $(shell find $(LIBK_X86_64_PATH) -name "*.asm")
AXON_SOURCE_X86_64 := $(shell find $(AXON_X86_64_PATH) -name "*.asm")

################################################# Building Object Paths #################################################

LIBC_OBJECTS_C := $(patsubst $(LIBC_SOURCE_PATH)%, $(LIBC_BUILD_PATH)%.o, $(LIBC_SOURCE_C) )
LIBK_OBJECTS_C :=  $(patsubst $(LIBK_SOURCE_PATH)%, $(LIBK_BUILD_PATH)%.o, $(LIBK_SOURCE_C) )
AXON_OBJECTS_C := $(patsubst $(AXON_SOURCE_PATH)%, $(AXON_BUILD_PATH)%.o, $(AXON_SOURCE_C) )

LIBC_OBJECTS_X86_64 := $(patsubst $(LIBC_X86_64_PATH)%.asm, $(LIBC_X86_64_BUILD_PATH)%.o, $(LIBC_SOURCE_X86_64) )
LIBK_OBJECTS_X86_64 := $(patsubst $(LIBK_X86_64_PATH)%.asm, $(LIBK_X86_64_BUILD_PATH)%.o, $(LIBK_SOURCE_X86_64) )
AXON_OBJECTS_X86_64 := $(patsubst $(AXON_X86_64_PATH)%.asm, $(AXON_X86_64_BUILD_PATH)%.o, $(AXON_SOURCE_X86_64) )

################################################# Build Source Code #################################################

$(LIBC_OBJECTS_C) : $(LIBC_SOURCE_C)
	mkdir -p $(dir $@) && \
	$(C_CC) $(LIBC_CPARAMS) $(patsubst $(LIBC_BUILD_PATH)%.o, $(LIBC_SOURCE_PATH)%, $@) -o $@

.PHONY: compile-libc-c
compile-libc-c: $(LIBC_OBJECTS_C)

$(LIBK_OBJECTS_C) : $(LIBK_SOURCE_C)
	mkdir -p $(dir $@) && \
	$(C_CC) $(LIBK_CPARAMS) $(patsubst $(LIBK_BUILD_PATH)%.o, $(LIBK_SOURCE_PATH)%, $@) -o $@

.PHONY: compile-libk-c
compile-libk-c: $(LIBK_OBJECTS_C)

$(AXON_OBJECTS_C) : $(AXON_SOURCE_C)
	mkdir -p $(dir $@) && \
	$(C_CC) $(AXON_CPARAMS) $(patsubst $(AXON_BUILD_PATH)%.o, $(AXON_SOURCE_PATH)%, $@) -o $@

.PHONY: compile-axon-c
compile-axon-c: $(AXON_OBJECTS_C)

################################################# Build x86_64 Code #################################################

$(LIBC_OBJECTS_X86_64) : $(LIBC_SOURCE_X86_64)
	mkdir -p $(dir $@) && \
	$(ASM_CC) $(LIBC_ASMPARAMS) $(patsubst $(LIBC_X86_64_BUILD_PATH)%.o, $(LIBC_X86_64_PATH)%.asm, $@) -o $@
	
.PHONY: compile-libc-asm-x86_64
compile-libc-asm-x86_64: $(LIBC_OBJECTS_X86_64)

$(LIBK_OBJECTS_X86_64) : $(LIBK_SOURCE_X86_64)
	mkdir -p $(dir $@) && \
	$(ASM_CC) $(LIBK_ASMPARAMS) $(patsubst $(LIBK_X86_64_BUILD_PATH)%.o, $(LIBK_X86_64_PATH)%.asm, $@) -o $@
	
.PHONY: compile-libk-asm-x86_64
compile-libk-asm-x86_64: $(LIBK_OBJECTS_X86_64)

$(AXON_OBJECTS_X86_64) : $(AXON_SOURCE_X86_64)
	mkdir -p $(dir $@) && \
	$(ASM_CC) $(AXON_ASMPARAMS) $(patsubst $(AXON_X86_64_BUILD_PATH)%.o, $(AXON_X86_64_PATH)%.asm, $@) -o $@
	
.PHONY: compile-axon-asm-x86_64
compile-axon-asm-x86_64: $(AXON_OBJECTS_X86_64)

################################################# Linking & Static Libs #################################################

build-libc-x86_64: compile-libc-asm-x86_64 compile-libc-c
	mkdir -p $(dir $(LIBC_OUTPUT_DIR)) && \
	$(ARCHIVER) $(LIBC_ARCPARAMS) $(LIBC_OUTPUT_DIR)/libc.a $(LIBC_OBJECTS_X86_64) $(LIBC_OBJECTS_C)

build-libk-x86_64: compile-libk-asm-x86_64 compile-libk-c
	mkdir -p $(dir $(LIBK_OUTPUT_DIR)) && \
	$(ARCHIVER) $(LIBK_ARCPARAMS) $(LIBK_OUTPUT_DIR)/libk.a $(LIBK_OBJECTS_X86_64) $(LIBK_OBJECTS_C)

build-axon-x86_64: compile-axon-asm-x86_64 compile-axon-c
	mkdir -p $(dir $(AXON_OUTPUT_DIR)) && \
	$(LINKER) $(AXON_LPARAMS) -o $(AXON_OUTPUT_DIR)axon.bin -L $(LIBK_OUTPUT_DIR) -T $(AXON_LINKFILE) $(AXON_OBJECTS_X86_64) $(AXON_OBJECTS_C) -l k


################################################# Main Commands #################################################

build-grub-bootable: build-libk-x86_64 build-axon-x86_64
	mkdir -p $(AXON_OUTPUT_DIR)iso/boot/ && \
	cp $(AXON_OUTPUT_DIR)axon.bin $(AXON_OUTPUT_DIR)iso/boot/axon.bin && \
	grub-mkrescue -d /usr/lib/grub/i386-pc --modules=ls -o $(AXON_OUTPUT_DIR)axon.iso $(AXON_OUTPUT_DIR)iso

.PHONY: clean-axon
clean-axon:
	rm -f -r $(AXON_BUILD_PATH)* && \
	rm -f $(AXON_OUTPUT_DIR)axon.bin && \
	rm -f $(AXON_OUTPUT_DIR)iso/boot/axon.bin

.PHONY: clean-libc
clean-libc:
	rm -f -r $(LIBC_BUILD_PATH)* && \
	rm -f $(LIBC_OUTPUT_DIR)libc.a

.PHONY: clean-libk
clean-libk:
	rm -f -r $(LIBK_BUILD_PATH)* && \
	rm -f $(LIBK_OUTPUT_DIR)libk.a

.PHONY: clean-all
clean-all: clean-libc clean-libk clean-axon

.PHONY: all
all: build-libc-x86_64 build-grub-bootable

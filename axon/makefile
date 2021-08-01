##########################################################################
# 	ExpressOS - Axon Kernel
#	Kernel Makefile
#	2021, Zachary Berry
##########################################################################


################################################# Paths #################################################

AXON_INCLUDE_PATH_PUBLIC 	= public/
AXON_INCLUDE_PATH_PRIVATE 	= private/
AXON_SOURCE_PATH 			= source/
AXON_X86_PATH 				= arch_x86/

AXON_BUILD_PATH 		= build/
AXON_X86_BUILD_PATH 	= build/x86/
AXON_OUTPUT_DIR 		= ../bin/x86/axon/

LIBK_OUTPUT_DIR			= ../bin/x86/libk/
LIBK_ROOT 				= ../libk/

LIBK_INCLUDE_PATH_PRIVATE 	= $(LIBK_ROOT)/private/
LIBK_INCLUDE_PATH_PUBLIC 	= $(LIBK_ROOT)/public/

############################################## Parameters ##############################################

C_CC 	= /usr/local/cross/bin/x86_64-elf-gcc
ASM_CC 	= nasm
LINKER 	= x86_64-elf-ld

AXON_CPARAMS ?= -O2 -g -std=c17
AXON_CPARAMS += -c -I $(AXON_INCLUDE_PATH_PRIVATE) -I $(AXON_INCLUDE_PATH_PUBLIC) -I $(LIBK_INCLUDE_PATH_PRIVATE) -I $(LIBK_INCLUDE_PATH_PUBLIC) -ffreestanding -nostdlib -lgcc -D _X86_64_ -mno-red-zone -mcmodel=kernel

AXON_ASMPARAMS ?=
AXON_ASMPARAMS += -f elf64

AXON_LPARAMS 	?= -n
AXON_LPARAMS 	+=
AXON_LINKFILE 	= arch_x86/linker.ld

############################################## Souce & Objects ##############################################

AXON_SOURCE_C 		:= $(shell find $(AXON_SOURCE_PATH) -type f -name "*.c")
AXON_SOURCE_X86 	:= $(shell find $(AXON_X86_PATH) -name "*.asm")
AXON_OBJECTS_C 		:= $(patsubst $(AXON_SOURCE_PATH)%, $(AXON_BUILD_PATH)%.o, $(AXON_SOURCE_C) )
AXON_OBJECTS_X86 	:= $(patsubst $(AXON_X86_PATH)%.asm, $(AXON_X86_BUILD_PATH)%.o, $(AXON_SOURCE_X86) )

################################################## Scripts #################################################
#
# 	build-axon-x86	=> 	Builds the x86_64 version of the kernel, outputting 'axon.bin'
#	clean-axon		=> 	Cleans all intermediate and output files from the axon build scripts
#

$(AXON_OBJECTS_C) : $(AXON_SOURCE_C)
	mkdir -p $(dir $@) && \
	$(C_CC) $(AXON_CPARAMS) $(patsubst $(AXON_BUILD_PATH)%.o, $(AXON_SOURCE_PATH)%, $@) -o $@

.PHONY: compile-axon-c
compile-axon-c: $(AXON_OBJECTS_C)

$(AXON_OBJECTS_X86) : $(AXON_SOURCE_X86)
	mkdir -p $(dir $@) && \
	$(ASM_CC) $(AXON_ASMPARAMS) $(patsubst $(AXON_X86_BUILD_PATH)%.o, $(AXON_X86_PATH)%.asm, $@) -o $@
	
.PHONY: compile-axon-asm-x86
compile-axon-asm-x86: $(AXON_OBJECTS_X86)

.PHONY: build-libk-x86
build-libk-x86:
	cd $(LIBK_ROOT) && make build-libk-x86

.PHONY: build-axon-x86
build-axon-x86: compile-axon-asm-x86 compile-axon-c build-libk-x86
	mkdir -p $(dir $(AXON_OUTPUT_DIR)) && \
	$(LINKER) $(AXON_LPARAMS) -o $(AXON_OUTPUT_DIR)axon.bin -L $(LIBK_OUTPUT_DIR) -T $(AXON_LINKFILE) $(AXON_OBJECTS_X86) $(AXON_OBJECTS_C) -l k

.PHONY: clean-axon
clean-axon:
	rm -f -r $(AXON_BUILD_PATH)* && \
	rm -f $(AXON_OUTPUT_DIR)axon.bin

# Old script for reference.. TODO: DELETE THIS
#build-grub-bootable: build-libk-x86_64 build-axon-x86_64
#	mkdir -p $(AXON_OUTPUT_DIR)iso/boot/ && \
#	cp $(AXON_OUTPUT_DIR)axon.bin $(AXON_OUTPUT_DIR)iso/boot/axon.bin && \
#	grub-mkrescue --modules=ls -o $(AXON_OUTPUT_DIR)axon.iso $(AXON_OUTPUT_DIR)iso



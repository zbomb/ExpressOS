##########################################################################
# 	ExpressOS - libc
#	Libc Makefile
#	2021, Zachary Berry
##########################################################################

################################################# Paths #################################################

LIBC_INCLUDE_PATH_PUBLIC 	= public/
LIBC_INCLUDE_PATH_PRIVATE 	= private/
LIBC_SOURCE_PATH 			= source/
LIBC_BUILD_PATH 			= build/

LIBC_X86_PATH 				= arch_x86/
LIBC_X86_BUILD_PATH 		= build/x86/
LIBC_OUTPUT_DIR 			= ../bin/x86/libc/

############################################## Parameters ##############################################

C_CC 		= /usr/local/cross/bin/x86_64-elf-gcc
ASM_CC 		= nasm
LINKER 		= x86_64-elf-ld
ARCHIVER 	= ar

LIBC_CPARAMS 	?= -O2 -g -std=c17
LIBC_CPARAMS 	+= -c -I $(LIBC_INCLUDE_PATH_PRIVATE) -I $(LIBC_INCLUDE_PATH_PUBLIC) -nostdlib -D _X86_64_ -lgcc

LIBC_ASMPARAMS 	?=
LIBC_ASMPARAMS 	+= -f elf64

LIBC_ARCPARAMS ?= 
LIBC_ARCPARAMS := rcs $(LIBC_ARCPARAMS)

############################################## Souce & Objects ##############################################

LIBC_SOURCE_C 			:= $(shell find $(LIBC_SOURCE_PATH) -type f -name "*.c")
LIBC_SOURCE_X86 		:= $(shell find $(LIBC_X86_PATH) -name "*.asm")
LIBC_OBJECTS_C 			:= $(patsubst $(LIBC_SOURCE_PATH)%, $(LIBC_BUILD_PATH)%.o, $(LIBC_SOURCE_C) )
LIBC_OBJECTS_X86 		:= $(patsubst $(LIBC_X86_PATH)%.asm, $(LIBC_X86_BUILD_PATH)%.o, $(LIBC_SOURCE_X86) )

################################################## Scripts #################################################
#
#	build-libc-x86	=> 	Builds the libc library, using the archiver to output 'libc.a'
#	clean-libc		=> 	Cleans the intermediate, and final output (libc.a) from the libc build scripts
#

$(LIBC_OBJECTS_C) : $(LIBC_SOURCE_C)
	mkdir -p $(dir $@) && \
	$(C_CC) $(LIBC_CPARAMS) $(patsubst $(LIBC_BUILD_PATH)%.o, $(LIBC_SOURCE_PATH)%, $@) -o $@

.PHONY: compile-libc-c
compile-libc-c: $(LIBC_OBJECTS_C)

$(LIBC_OBJECTS_X86) : $(LIBC_SOURCE_X86)
	mkdir -p $(dir $@) && \
	$(ASM_CC) $(LIBC_ASMPARAMS) $(patsubst $(LIBC_X86_BUILD_PATH)%.o, $(LIBC_X86_PATH)%.asm, $@) -o $@
	
.PHONY: compile-libc-asm-x86
compile-libc-asm-x86: $(LIBC_OBJECTS_X86)

build-libc-x86: compile-libc-asm-x86 compile-libc-c
	mkdir -p $(dir $(LIBC_OUTPUT_DIR)) && \
	$(ARCHIVER) $(LIBC_ARCPARAMS) $(LIBC_OUTPUT_DIR)/libc.a $(LIBC_OBJECTS_X86) $(LIBC_OBJECTS_C)

.PHONY: clean-libc
clean-libc:
	rm -f -r $(LIBC_BUILD_PATH)* && \
	rm -f $(LIBC_OUTPUT_DIR)libc.a
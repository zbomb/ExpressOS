##########################################################################
# 	ExpressOS - libk
#	Libk Makefile
#	2021, Zachary Berry
##########################################################################


################################################# Paths #################################################

LIBK_INCLUDE_PATH_PUBLIC 	= public/
LIBK_INCLUDE_PATH_PRIVATE 	= private/
LIBK_SOURCE_PATH 			= source/
LIBK_BUILD_PATH 			= build/

LIBK_X86_PATH 				= arch_x86/
LIBK_X86_BUILD_PATH 		= build/x86/
LIBK_OUTPUT_DIR 			= ../bin/x86/libk/

############################################## Parameters ##############################################

C_CC 		= /usr/local/cross/bin/x86_64-elf-gcc
ASM_CC 		= nasm
LINKER 		= x86_64-elf-ld
ARCHIVER 	= ar

LIBK_CPARAMS 	?= -O2 -g -std=c17
LIBK_CPARAMS 	+= -c -I $(LIBK_INCLUDE_PATH_PRIVATE) -I $(LIBK_INCLUDE_PATH_PUBLIC) -nostdlib -D _X86_64_ -lgcc

LIBK_ASMPARAMS 	?=
LIBK_ASMPARAMS 	+= -f elf64

LIBK_ARCPARAMS ?= 
LIBK_ARCPARAMS := rcs $(LIBK_ARCPARAMS)

############################################## Souce & Objects ##############################################

LIBK_SOURCE_C 			:= $(shell find $(LIBK_SOURCE_PATH) -type f -name "*.c")
LIBK_SOURCE_X86 		:= $(shell find $(LIBK_X86_PATH) -name "*.asm")
LIBK_OBJECTS_C 			:= $(patsubst $(LIBK_SOURCE_PATH)%, $(LIBK_BUILD_PATH)%.o, $(LIBK_SOURCE_C) )
LIBK_OBJECTS_X86 		:= $(patsubst $(LIBK_X86_PATH)%.asm, $(LIBK_X86_BUILD_PATH)%.o, $(LIBK_SOURCE_X86) )

################################################## Scripts #################################################
#
#	build-libk-x86	=> 	Builds the libk library, using the archiver to output 'libk.a'
#	clean-libk		=> 	Cleans the intermediate, and final output (libk.a) from the libk build scripts
#

$(LIBK_OBJECTS_C) : $(LIBK_SOURCE_C)
	mkdir -p $(dir $@) && \
	$(C_CC) $(LIBK_CPARAMS) $(patsubst $(LIBK_BUILD_PATH)%.o, $(LIBK_SOURCE_PATH)%, $@) -o $@

.PHONY: compile-libk-c
compile-libk-c: $(LIBK_OBJECTS_C)

$(LIBK_OBJECTS_X86) : $(LIBK_SOURCE_X86)
	mkdir -p $(dir $@) && \
	$(ASM_CC) $(LIBK_ASMPARAMS) $(patsubst $(LIBK_X86_BUILD_PATH)%.o, $(LIBK_X86_PATH)%.asm, $@) -o $@
	
.PHONY: compile-libk-asm-x86
compile-libk-asm-x86: $(LIBK_OBJECTS_X86)

build-libk-x86: compile-libk-asm-x86 compile-libk-c
	mkdir -p $(dir $(LIBK_OUTPUT_DIR)) && \
	$(ARCHIVER) $(LIBK_ARCPARAMS) $(LIBK_OUTPUT_DIR)/libk.a $(LIBK_OBJECTS_X86) $(LIBK_OBJECTS_C)

.PHONY: clean-libk
clean-libk:
	rm -f -r $(LIBK_BUILD_PATH)* && \
	rm -f $(LIBK_OUTPUT_DIR)libk.a
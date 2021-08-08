##########################################################################
# 	ExpressOS - T-0 Bootloader
#	T-0 Makefile
#	2021, Zachary Berry
##########################################################################


################################################# Paths #################################################

TZERO_INCLUDE_PATH_X86 	= include/arch_x86/
TZERO_SOURCE_PATH_X86	= arch_x86/
TZERO_BUILD_PATH_X86 	= build/x86/
TZERO_OUTPUT_DIR_X86 	= ../bin/x86/t-zero/
TZERO_DATA_PATH_X86		= data/x86/

GNU_EFI_INCLUDE_PATH 			= /usr/include/efi/
GNU_EFI_X86_64_INCLUDE_PATH 	= /usr/include/efi/x86_64/
GNU_EFI_PROTOCOL_INCLUDE_PATH 	= /usr/include/efi/protocol/

############################################## Parameters ##############################################

C_CC 	= /usr/local/cross/bin/x86_64-elf-gcc
EFI_CC 	= /usr/bin/x86_64-w64-mingw32-gcc

TZERO_CPARAMS_X86 ?= -std=c11 
TZERO_CPARAMS_X86 += -c -ffreestanding -I $(GNU_EFI_INCLUDE_PATH) -I $(GNU_EFI_X86_64_INCLUDE_PATH) -I $(GNU_EFI_PROTOCOL_INCLUDE_PATH) -I $(TZERO_INCLUDE_PATH_X86) -D __x86_64__ -fpic -fno-stack-protector -fno-stack-check -mno-red-zone -maccumulate-outgoing-args

TZERO_LFLAGS_X86 = -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main 

############################################## Souce & Objects ##############################################

TZERO_SOURCE_C_X86 			:= $(shell find $(TZERO_SOURCE_PATH_X86) -type f -name "*.c")
TZERO_OBJECTS_C_X86_OS 		:= $(patsubst $(TZERO_SOURCE_PATH_X86)%, $(TZERO_BUILD_PATH_X86)%.os.o, $(TZERO_SOURCE_C_X86) )
TZERO_OBJECTS_C_X86_INST 	:= $(patsubst $(TZERO_SOURCE_PATH_X86)%, $(TZERO_BUILD_PATH_X86)%.inst.o, $(TZERO_SOURCE_C_X86) )

################################################## Scripts #################################################
#
#	build-tzero-os-x86			=> 	Build the x86 version of the bootloader, that will boot an OS installation
#	build-tzero-installer-x86	=> 	Build the x86 version of the bootloader, that will boot an installer program
#	clean-tzero					=>	Cleans intermediate and output files from the t-zero build scripts
#

$(TZERO_OBJECTS_C_X86_OS) : $(TZERO_SOURCE_C_X86)
	mkdir -p $(dir $@) && \
	$(EFI_CC) $(TZERO_CPARAMS_X86) -D TZERO_TARGET_OS $(patsubst $(TZERO_BUILD_PATH_X86)%.os.o, $(TZERO_SOURCE_PATH_X86)%, $@) -o $@

$(TZERO_OBJECTS_C_X86_INST) : $(TZERO_SOURCE_C_X86)
	mkdir -p $(dir $@) && \
	$(EFI_CC) $(TZERO_CPARAMS_X86) -D TZERO_TARGET_INSTALLER $(patsubst $(TZERO_BUILD_PATH_X86)%.isnt.o, $(TZERO_SOURCE_PATH_X86)%, $@) -o $@

.PHONY: compile-tzero-os-x86
compile-tzero-os-x86: $(TZERO_OBJECTS_C_X86_OS)

.PHONY: compile-tzero-installer-x86
compile-tzero-installer-x86: $(TZERO_OBJECTS_C_X86_INST)

.PHONY: build-tzero-os-x86
build-tzero-os-x86: compile-tzero-os-x86

.PHONY: build-tzero-installer-x86
build-tzero-installer-x86: compile-tzero-installer-x86

.PHONY: build-tzero-debug-x86
build-tzero-debug-x86: compile-tzero-os-x86
	mkdir -p $(dir $(TZERO_BUILD_PATH_X86))debug_iso/ && \
	rm -r -f $(dir $(TZERO_BUILD_PATH_X86))debug_iso/* && \
	mkdir -p $(dir $(TZERO_BUILD_PATH_X86))debug_iso/EFI/BOOT/ && \
	mkdir -p $(dir $(TZERO_OUTPUT_DIR_X86))os_launcher/ && \
	cp -r $(TZERO_DATA_PATH_X86)* $(TZERO_BUILD_PATH_X86)debug_iso && \
	$(EFI_CC) $(TZERO_LFLAGS_X86) -o $(TZERO_OUTPUT_DIR_X86)os_launcher/BOOTX64.EFI $(TZERO_OBJECTS_C_X86_OS) && \
	cp $(TZERO_OUTPUT_DIR_X86)os_launcher/BOOTX64.EFI $(TZERO_BUILD_PATH_X86)debug_iso/EFI/BOOT/ && \
	xorriso -as mkisofs -R -f -no-emul-boot -o $(TZERO_OUTPUT_DIR_X86)tzero_debug.iso $(TZERO_BUILD_PATH_X86)debug_iso

.PHONY: clean-tzero
clean-tzero:
	rm -f -r $(TZERO_OUTPUT_DIR_X86)* && \
	rm -f -r $(TZERO_BUILD_PATH_X86)*
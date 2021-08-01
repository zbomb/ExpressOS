##########################################################################
# 	ExpressOS 
#	Project Makefile
#	2021, Zachary Berry
##########################################################################

################################################# Paths #################################################

AXON_ROOT 		= axon/
OS_ROOT 		= os-main/
TZERO_ROOT 		= t-zero/
INSTALLER_ROOT 	= installer/

################################################ Scripts ################################################

# OS Commands: 			build-os-[iso/img/install]-[x86/ARM/etc..]
# Installer Commands: 	build-installer-[iso/img]-[x86/ARM/etc..]
# Kernel Commands: 		build-axon-[x86/ARM/etc..]
# Bootloader Commands: 	build-tzero-[os/installer]-[x86/ARM/etc..]

#########################################################
#		Express OS Commands
#########################################################

.PHONY: build-express-iso-x86 	## TODO: Invoke xorriso to create iso
build-express-iso-x86:
	cd $(AXON_ROOT) && make build-axon-x86 && \
	cd $(TZERO_ROOT) && make build-tzero-os-x86 && \
	cd $(OS_ROOT) && make build-express-x86 

.PHONY: build-express-img-x86 	## TODO: Create FAT32 image, we might remove this command in the future
build-express-img-x86:
	cd $(AXON_ROOT) && make build-axon-x86 && \
	cd $(TZERO_ROOT) && make build-tzero-os-x86 && \
	cd $(OS_ROOT) && make build-express-x86 

.PHONY: build-express-install-x86	## TODO: Package the output
build-express-install-x86:
	cd $(AXON_ROOT) && make build-axon-x86 && \
	cd $(TZERO_ROOT) && make build-tzero-os-x86 && \
	cd $(OS_ROOT) && make build-express-x86 

.PHONY: clean-express
clean-express:
	cd $(OS_ROOT) && make clean-express

#########################################################
#		Installer Commands
#########################################################

.PHONY: build-installer-iso-x86		## TODO: Invoke xorriso to build final output
build-installer-iso-x86:
	cd $(TZERO_ROOT) && make build-tzero-installer-x86 && \
	cd $(INSTALLER_ROOT) && make-build-installer-x86 

.PHONY: build-installer-img-x86		## TODO: Create FAT32 image final output
build-installer-img-x86:
	cd $(TZERO_ROOT) && make build-tzero-installer-x86 && \
	cd $(INSTALLER_ROOT) && make-build-installer-x86 

.PHONY: clean-installer
clean-installer:
	cd $(INSTALLER_ROOT) && make clean-installer

#########################################################
#		Axon Kernel Commands
#########################################################

.PHONY: build-axon-x86
build-axon-x86:
	cd $(AXON_ROOT) && make build-axon-x86

.PHONY: clean-axon
clean-axon:
	cd $(AXON_ROOT) && make clean-axon

.PHONY: build-axon-efi-test
build-axon-efi-test:
	make build-axon-x86 && \
	cp bin/x86/axon/axon.bin t-zero/data/x86/ && \
	make build-tzero-debug-x86

#########################################################
#		T-0 Bootloader Commands
#########################################################

.PHONY: build-tzero-os-x86
build-tzero-os-x86:
	cd $(TZERO_ROOT) && make build-tzero-os-x86

.PHONY: build-tzero-installer-x86
build-tzero-installer-x86:
	cd $(TZERO_ROOT) && make build-tzero-installer-x86

.PHONY: build-tzero-debug-x86
build-tzero-debug-x86:
	cd $(TZERO_ROOT) && make build-tzero-debug-x86

.PHONY: clean-tzero
clean-tzero:
	cd $(TZERO_ROOT) && make clean-tzero


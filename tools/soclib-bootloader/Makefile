#=====================================================================
#                      AlmOS BootLoader Makefile                     #
#=====================================================================

# CPU type
CPU=$(ALMOS_CPU)

# ARCH type
ARCH=$(ALMOS_ARCH)

# Kernel bin directory
TARGET_DIR=$(ALMOS_TOP)/tools/soclib-bootloader

# CPU directory
CPU_DIR=$(CPU)

# System source code dirctory 
SYS_DIR=$(ALMOS_TOP)/kernel

# ARCH directory
ARCH_DIR=$(ALMOS_ARCH)


# bootloader directories contain source files
DIRS = arch common $(CPU_DIR) libk

# arch-info name (to be put into $ALMOS_TOP)
ARCH_INFO ?=arch-info_4_4_mipsel.bin

# kernel directory : must contain obj.$(ARCH) directory
# which must contain the kernel image : 
KERNEL_DIR ?= $(ALMOS_TOP)/kernel


ifeq ($(MODE),DEBUG)
EXTRACFLAGS = -g -O0 -fno-inline
else
EXTRACFLAGS = -O2 -fomit-frame-pointer
endif

# CPU specific flags
ifeq ($(CPU), mipsel)
CPUCFLAGS = -mips32 -EL -G0
CPULFLAGS = -G0 --hash-style=sysv
endif

# Compilation flags
CFLAGS = -I. -c -fno-builtin -c -static -Wall -Werror $(CPUCFLAGS) $(EXTRACFLAGS)
CFLAGS += -Wno-unused-variable

# LD flags
LFLAGS = -T$(BUILD_DIR)/ldscript $(CPULFLAGS)

# Add directories to the include and library paths
INCPATH = $(DIRS) $(BUILD_DIR)/libk $(SYS_DIR)/libk

# Which files to add to backups, apart from the source code
EXTRA_FILES = Makefile

# Hexdump utility
HEXDUMP = hexdump

# CC tools and parameters
#------------------------------------------------------------------------------
CC              = $(CCTOOLS)/bin/$(CPU)-unknown-elf-gcc
LD              = $(CCTOOLS)/bin/$(CPU)-unknown-elf-ld
NM              = $(CCTOOLS)/bin/$(CPU)-unknown-elf-nm
STRIP           = $(CCTOOLS)/bin/$(CPU)-unknown-elf-strip 

# Where to store object and dependancy files
BUILD_DIR = .build

# Makes a list of the source (.c) files
SOURCE_C:= $(foreach DIR,$(DIRS),$(wildcard $(DIR)/*.c))
#SOURCE_S:= $(shell echo $(CPU_DIR)/*.S)
SOURCES := $(SOURCE_C)# $(SOURCE_S)

# List of header files
HEADERS := $(foreach DIR,$(DIRS),$(wildcard $(DIR)/*.h))

# Makes a list of the object files that will have to be created
OBJECTS_C := $(addprefix $(BUILD_DIR)/, $(SOURCE_C:.c=.o))
#OBJECTS_S := $(addprefix $(BUILD_DIR)/, $(patsubst %.S,%.o,$(SOURCE_S)))
OBJECTS   := $(OBJECTS_C)# $(OBJECTS_S)

# Same for the .d (dependancy) files
DFILES_C := $(addprefix $(BUILD_DIR)/,$(SOURCE_C:.c=.d))
#DFILES_S := $(addprefix $(BUILD_DIR)/,$(SOURCE_S:.S=.d))
DFILES   := $(DFILES_C) $(DFILES_S)

# Compilation date header
ALMOS_DATE=$(BUILD_DIR)/libk/almOS-date.h
ALMOS_DEP=$(BUILD_DIR)/libk/almOS.dep

# Bootloeader bin file name
TARGET=$(TARGET_DIR)/bootloader-$(ARCH)-$(CPU).bin

# Rules that are not real files
.PHONY: clean realclean dirs start backup

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
$(TARGET): start $(OBJECTS) $(BUILD_DIR)/ldscript
	@echo '   [  LD  ]        '$@
	$(LD) -o $@ $(OBJECTS) $(LFLAGS)
	@echo '   [  NM  ]        '$@.nm 
	@$(NM) $@ > $@.nm
	@echo -e '\n   [ DONE ]        \n'

start: dirs
	@echo  -e '\nAlmOS Bootloader Compilation Process ...\n'
#	$(shell echo '#define ALMOS_DATE "'$(shell date +"%d %B %Y - %T")'"' > $(ALMOS_DATE))
	$(shell echo '#define ALMOS_DATE "'$(shell date +"%d %B %Y")'"' > $(ALMOS_DATE))

$(BUILD_DIR)/ldscript : common/ldscript.h $(BUILD_DIR)/arch-info.ld $(BUILD_DIR)/kernel-img.ld
	@echo '   [  CPP ]        '$< 
	@cpp -I$(ARCH_DIR) $< | grep -v "#" | grep . > $@

#convert binary files into format compatible with the linker
$(BUILD_DIR)/arch-info.ld : $(ALMOS_TOP)/$(ARCH_INFO)
	@echo '  [ HEXDUMP ]   '$@
	$(HEXDUMP) -v -e '"BYTE(0x" 1/1 "%02X" ")\n"' $< > $@
	
$(BUILD_DIR)/kernel-img.ld : $(KERNEL_DIR)/obj.$(ARCH)/almos-mk-$(ARCH)-$(CPU).bin
	@echo '  [ HEXDUMP ]   '$@
	$(HEXDUMP) -v -e '"BYTE(0x" 1/1 "%02X" ")\n"' $< > $@

# Rule for creating object file and .d file, the sed magic is to add
# the object path at the start of the file because the files gcc
# outputs assume it will be in the same dir as the source file
$(BUILD_DIR)/%.o: %.c
	@echo '   [  CC  ]  $(foreach INC,$(INCPATH),-I$(INC))'$<
	$(CC) -Wp,-MMD,$(BUILD_DIR)/$*.dd $(CFLAGS) $(foreach INC,$(INCPATH),-I$(INC)) -c $< -o $@
ifneq   ($(MODE), DEBUG)
	#@$(STRIP)-x -R .pdr -R .gnu.attributes -R .reginfo -R .comment $@
endif
	@sed -e '1s/^\(.*\)$$/$(subst /,\/,$(dir $@))\1/' $(BUILD_DIR)/$*.dd > $(BUILD_DIR)/$*.d
	@$(RM) $(BUILD_DIR)/$*.dd

$(BUILD_DIR)/%.o: %.S
	@echo '   [  AS  ]        '$<
	$(CC) -Wp,-MMD,$(BUILD_DIR)/$*.dd $(CFLAGS) $(foreach INC,$(INCPATH),-I$(INC)) -c $< -o $@
ifneq   ($(MODE), DEBUG)
	#@$(STRIP)-x -R .pdr -R .gnu.attributes -R .reginfo -R .comment $@
endif
	@sed -e '1s/^\(.*\)$$/$(subst /,\/,$(dir $@))\1/' $(BUILD_DIR)/$*.dd > $(BUILD_DIR)/$*.d
	@$(RM) $(BUILD_DIR)/$*.dd


# Empty rule to prevent problems when a header is deleted
%.h: ;

# Cleans up the objects, .d files, kldscript and executables
clean:
	@$(RM) $(foreach DIR,$(DIRS),$(BUILD_DIR)/$(DIR)/*.d $(BUILD_DIR)/$(DIR)/*.o)
	@$(RM) $(BUILD_DIR)/ldscript 
	@$(RM) $(BUILD_DIR)/arch-info.ld
	@$(RM) $(BUILD_DIR)/kernel-img.ld
	@$(RM) $(TARGET) $(TARGET).nm

realclean: clean
	@$(RM) -r $(BUILD_DIR)

# Backup the source files
backup:
	@if [ ! -e .backup ]; then mkdir .backup; fi;
	@tar jcvf .backup/backup_`date +%d-%m-%y_%H.%M`.tbz2 $(SOURCES) $(HEADERS) $(EXTRA_FILES)


# Create compilation tree
dirs:
	@if [ ! -e $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi;
	@$(foreach DIR,$(DIRS), 					\
		if [ ! -e $(BUILD_DIR)/$(DIR) ]; 			\
		then 							\
			echo '   [ MKDIR ]       '$(BUILD_DIR)/$(DIR);  \
			mkdir $(BUILD_DIR)/$(DIR); 			\
		fi;)

# Includes the .d files so it knows the exact dependencies for every source
-include $(DFILES)

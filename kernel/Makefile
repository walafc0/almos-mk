#=====================================================================
#                      ALMOS-kernel Makefile                         #
#=====================================================================

include ../common.mk

KERNEL_REV ?=current

DIRS=	cpu/$(CPU)           	  \
	arch/$(ARCH)         	  \
	drivers/$(DRVRS)	  \
	kern                      \
	mm                        \
	fs/devfs                  \
	fs/sysfs                  \
	fs/ext2                   \
	fs/fat32                  \
	vfs                       \
	libk                      \
	ksh

ifndef CPU
$(error CPU variable is not defined)
endif

LFLAGS = -T$(OBJDIR)/kldscript $(CPULFLAGS)

INCLUDES=	$(foreach DIR,$(DIRS),-I$(DIR)) -I$(OBJDIR)/kern/

CFLAGS  = 	$(INCLUDES) -fno-builtin -static -Wall -Werror -MMD -Wundef
ifeq   ($(MODE),DEBUG)
CFLAGS  += 	-g -O0 -fno-inline
else
CFLAGS  += 	-O2 
endif
CFLAGS +=	$(CPUCFLAGS)

CFILES=	$(foreach DIR,$(DIRS),$(wildcard $(DIR)/*.c))
SFILES=	$(wildcard cpu/$(CPU)/*.S)

OBJS=	$(addprefix $(OBJDIR)/, $(CFILES:.c=.o) $(SFILES:.S=.o))
DEPS=	$(OBJS:.o=.d)

OBJDIRS= $(addprefix $(OBJDIR)/, $(DIRS))

# Kernel bin file name
ifeq ($(KERNEL_REV),current)
TARGET=$(OBJDIR)/almos-mk-$(ARCH)-$(CPU)
else
TARGET=$(OBJDIR)/almos-mk-$(ARCH)-$(CPU)-$(KERNEL_REV)
endif


.PHONY: clean timestamp


$(TARGET): $(OBJDIRS) $(OBJDIR)/kldscript timestamp $(OBJS) 
	@echo '   [  LD  ]        '$@.elf
	@$(LD) -o $@.elf $(OBJS) $(LFLAGS)
	@echo '   [  NM  ]        '$@.elf.nm 
	@$(NM) $@.elf > $@.elf.nm
	@echo '   [ COPY ]        '$@.bin
	@$(OCPY) -O binary  $@.elf  $@.bin
	@echo -e '\n   [ DONE ]        \n'
	$(ODMP) -d $@.elf > $@.dump


timestamp:
	@echo "Compile flags: $(CFLAGS)"
	@echo '#define ALMOS_DATE "'$(shell date +"%d %B %Y - %T")'"' > $(OBJDIR)/kern/almOS-date.h 

$(OBJDIR)/kldscript: arch/$(ARCH)/kldscript.h arch/$(ARCH)/arch-config.h
	@echo '   [  CPP ]        '$< 
	@cpp $< | grep -v "#" | grep . > $@

$(OBJDIR)/%.o: %.c
	@echo '   [  CC  ]        '$<
	cpp $(INCLUDES) $< out.cpp
	@$(CC) $(CFLAGS) -c $< -o $@
ifneq   ($(MODE),DEBUG)
	@$(STRIP) -x -R .pdr -R .gnu.attributes -R .reginfo -R .comment $@
endif

$(OBJDIR)/%.o: %.S
	@echo '   [  AS  ]        '$<
	@$(CC) $(CFLAGS) -c $< -o $@
ifneq   ($(MODE),DEBUG)
	@$(STRIP) -x -R .pdr -R .gnu.attributes -R .reginfo -R .comment $@
endif

clean:
	rm -rf $(OBJDIR) out.cpp

$(OBJDIRS):
	@echo '   [ MKDIR ]       '$@
	@mkdir -p $@


-include $(DEPS)

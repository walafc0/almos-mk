# Predefined User Makefile

#------------------------------------------------------------------------------
# Default plateform architecture and default CPU
#------------------------------------------------------------------------------
ARCH        = $(ALMOS_ARCH)
CPU         = $(ALMOS_CPU)
ARCH_CLASS  = $(ALMOS_ARCH_CLASS)

#------------------------------------------------------------------------------
# CC tools and parameters
#------------------------------------------------------------------------------
DIR_INC  = $(ALMOS_DISTRIB)/include
DIR_LIB  = $(ALMOS_DISTRIB)/lib
GCC_LIB  = $(CCTOOLS)/lib
CC       = $(CCTOOLS)/bin/$(CPU)-unknown-elf-gcc		
AR       = $(CCTOOLS)/bin/$(CPU)-unknown-elf-ar
AS       = $(CCTOOLS)/bin/$(CPU)-unknown-elf-as
OD       = $(CCTOOLS)/bin/$(CPU)-unknown-elf-objdump
OCPY	 = $(CCTOOLS)/bin/$(CPU)-unknown-elf-objcopy
LD       = $(CCTOOLS)/bin/$(CPU)-unknown-elf-ld
NM	 = $(CCTOOLS)/bin/$(CPU)-unknown-elf-nm

ifneq ($(ADD-LDSCRIPT),)
LDSCRIPT=$(ADD-LDSCRIPT)
else
LDSCRIPT=uldscript
endif

ifeq ($(CPU), mipsel)
CPU-CFLAGS = -mips32 -EL -G0
endif

ifeq ($(CPU), i386)
CPU-CFLAGS = -g
endif

CFLAGS = -c -fomit-frame-pointer $(ADD-CFLAGS)
LIBS =  -L$(DIR_LIB) -L$(GCC_LIB) $(CPU-LFLAGS) -T$(LDSCRIPT) $(OBJ) $(ADD-LIBS) \
	-lpthread -lgomp -lc $(ADD-LIBS) -lgomp -lpthread -lc -lgcc --hash-style=sysv

#-------------------------------------------------------------------------------
# Sources and targets files name
#-------------------------------------------------------------------------------
OBJ=$(addsuffix .o,$(FILES))
SRC=$(addsuffix .c,$(FILES))
BIN ?= soft.bin

ifndef TARGET
RULE=usage
endif

ifeq ($(TARGET), tsar)
RULE=almos
CFLAGS += $(CPU-CFLAGS) -I$(DIR_INC) -D_ALMOS_
endif

ifeq ($(TARGET), linux)
RULE=linux
CFLAGS += -g
CC=gcc
endif

ifeq ($(TARGET), ibmpc)
RULE=almos
CFLAGS += $(CPU-CFLAGS) -I$(DIR_INC) -D_ALMOS_
endif

ifndef RULE
RULE=usage
endif

#------------------------------------------------------------------------------
# Building rules
#------------------------------------------------------------------------------
.PHONY : usage linux soclib clean realclean

all: $(RULE) $(ADD-RULE)

usage:
	@echo "AlmOS Application Compiler";\
	echo "> make TARGET=tsar     : targets the MIPS TSAR simulator";\
	echo "> make TARGET=ibmpc    : targets the Intel X86 for IBM-PC compatible plateform";\
	echo "> make TARGET=linux    : targets the GNU/Linux plateform";\
	echo ""

linux: $(OBJ)
	@echo '   [  CC  ]        '$^ 
	@$(CC) -o $(BIN) $^ -lpthread $(ADD-LIBS)

almos : $(BIN)

$(BIN) : $(OBJ)
	@echo '   [  LD  ]        '$@
	@$(LD) -o $@ $(LIBS)

%.o : %.c
	@echo '   [  CC  ]        '$<
	@$(CC) $(CFLAGS) $< -o $@

clean :
	@echo '   [  RM  ]        *.o *~ *.dump *.nm '"$(BOJ)"
	@$(RM) $(OBJ) *.o *~ *.dump *.nm 2> /dev/null||true 

realclean : clean
	@echo '   [  RM  ]        '$(BIN) tty* vcitty* 
	@$(RM) $(BIN) tty* vcitty*


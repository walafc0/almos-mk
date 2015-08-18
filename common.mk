
ARCH ?= tsar
OBJDIR=obj.$(ARCH)

ifeq ($(ARCH),tsar)
CPU=mipsel

CPUCFLAGS=	-mips32 -EL -G0
CPUCFLAGS+=	-mhard-float -fomit-frame-pointer

CPULFLAGS= 	--hash-style=sysv

DRVRS=	soclib

CCPREFIX= $(CPU)-unknown-elf-
endif


ifeq ($(ARCH),ibmpc)
CPU=		i386
CPUCFLAGS= 	-march=i386
#CPUCFLAGS+=	-msoft-float -fomit-frame-pointer -Os

CCPREFIX=
endif


CC=	$(CCPREFIX)cc
AR=	$(CCPREFIX)ar
LD=	$(CCPREFIX)ld
NM=	$(CCPREFIX)nm
STRIP=	$(CCPREFIX)strip
OCPY=	$(CCPREFIX)objcopy
ODMP=	$(CCPREFIX)objdump

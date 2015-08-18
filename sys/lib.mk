#
# lib.mk - Common rules to build libraries
#

TGTDIR=almos-$(ARCH)-$(CPU)/almos

LIBDIR=$(ALMOS_DISTRIB)/lib
INCDIR=$(ALMOS_DISTRIB)/include

include $(SRCDIR)../../common.mk

ifneq ($(SRCDIR), )

CFLAGS = -W -Wall -Wextra -Wchar-subscripts -Werror \
	 -Wno-switch -Wno-unused -Wredundant-decls  \
	 -fno-strict-aliasing -fno-pic -static -O3  \
	 -DUSR_LIB_COMPILER -D_ALMOS_ -DZ_PREFIX -DHAVE_GOMP \
	 $(CPUCFLAGS) $(INCFLAGS) 

VPATH ?=$(SRCDIR)
OBJS   =$(filter %.o, $(SRCS:.c=.o) $(SRCS:.S=.o))


all: checktgt lib$(LIB).a install

lib$(LIB).a:: $(OBJS)
	@rm -f $@
	@$(AR) -r $@ $^

install:lib$(LIB).a
	@[ -d $(LIBDIR) ] || mkdir -p $(LIBDIR)
	@[ -d $(INCDIR) ] || mkdir -p $(INCDIR)
	cp -r $(SRCDIR)/include/* $(INCDIR)/.
	cp $^ $(LIBDIR)/.

checktgt:
	@if [ -z $$ALMOS_DISTRIB ]; then echo "ERROR: Missing ALMOS_DISTRIB macro, no target directory has been specified"; \
	exit 1; else exit 0; fi

clean:
	rm -f lib$(LIB).a $(OBJS)


# Set of rules to for a multi-architecture build.
#
# See http://make.paulandlesley.org/multi-arch.html for more information
else

MAKEFLAGS+= --no-print-directory

$(OBJDIR):
	@[ -d $@ ] || mkdir -p $@
	@cd $@ && $(MAKE) -f $(CURDIR)/Makefile SRCDIR=$(CURDIR)/ $(MAKECMDGOALS)

Makefile : ;

% :: $(OBJDIR) ;

clean:
	@rmdir $(OBJDIR)

.PHONY:	$(OBJDIR) clean
endif

TGTDIR=almos-$(ARCH)-$(CPU)/almos

LIBDIR=$(ALMOS_DISTRIB)/lib
INCDIR=$(ALMOS_DISTRIB)/include

LDSCRIPT=$(ARCH)_uldscript

include $(SRCDIR)../../common.mk



all:
	cp $(ARCH)_uldscript $(LIBDIR)/uldscript
	cp include/* $(INCDIR)/.

clean:
	@echo ""


.PHONY: clean all

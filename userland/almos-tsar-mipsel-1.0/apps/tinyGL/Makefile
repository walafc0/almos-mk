BIN=$(PROG)
FILES=$(BIN) glu soclibfb_almos
ADD-CFLAGS= -O2
ADD-LIBS=-lTinyGL -lm

HDD=$(ALMOS_TOP)/hdd-img.bin

default: check_prog check_target all

include $(ALMOS_USR_TOP)/include/appli.mk


check_prog:
	@if [ -z $$PROG ]; then echo "ERROR: Missing PROG macro, possible values are: gears, mech, spin or texobj"; \
        exit 1; else exit 0; fi

check_target:
	@if [ -z $$TARGET ]; then echo "ERROR: Missing TARGET macro, the only supported target is tsar"; \
        exit 1; else exit 0; fi
	@if [ $$TARGET != "tsar" ]; then echo "ERROR: the ui_loop() expects a specific access to soclib frame-buffer on ALMOS and therefore, it will not compile on Linux"; \
        exit 1; else exit 0; fi

install: check_prog
	mcopy -i $(HDD) $(BIN) ::bin/.

.PHONY: check_prog check_target install default

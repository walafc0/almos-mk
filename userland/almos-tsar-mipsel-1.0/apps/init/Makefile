FILES= main
BIN=init
ADD-CFLAGS=-O3

HDD=$(ALMOS_TOP)/hdd-img.bin

include $(ALMOS_USR_TOP)/include/appli.mk

clean : mproper

mproper:
	@rm -f kernel-soclib.bin soft.bin

install: $(BIN)
	mcopy $< -o -i $(HDD) ::bin/$<

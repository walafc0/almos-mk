FILES=main
BIN=bcv
ADD-CFLAGS=-O3

HDD=$(ALMOS_TOP)/hdd-img.bin

include $(ALMOS_USR_TOP)/include/appli.mk

install: $(BIN)
	mcopy -i $(HDD) $(BIN) ::bin/.

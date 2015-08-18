FILES=main
BIN=tomp
ADD-CFLAGS=-fopenmp -O3
ADD-LIBS=-lm

HDD=$(ALMOS_TOP)/hdd-img.bin

include $(ALMOS_USR_TOP)/include/appli.mk

install: $(BIN)
	mcopy -i $(HDD) $(BIN) ::bin/.

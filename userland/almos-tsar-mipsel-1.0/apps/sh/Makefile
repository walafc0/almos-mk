FILES= cat		\
       cd		\
       clear		\
       cp		\
       echo		\
       error		\
       exec_func	\
       export		\
       kill		\
       ls		\
       mkdir_func	\
       mkfifo_func	\
       printenv		\
       ps		\
       rm		\
       ush		\
       ushIo

BIN=sh
ADD-CFLAGS=-O2 -g
HDD=$(ALMOS_TOP)/hdd-img.bin


include $(ALMOS_USR_TOP)/include/appli.mk

clean : mproper

mproper:
	@rm -f kernel-soclib.bin soft.bin

install: $(BIN)
	mcopy $< -o -i $(HDD) ::bin/$<

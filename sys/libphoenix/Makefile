
LIB=	phoenix

SRCS=	iterator.c locality.c map_reduce.c \
	mcs.c memory.c processor.c pt_mutex.c \
	scheduler.c synch.c taskQ.c tpool.c

INCFLAGS= -I$(SRCDIR)/include -I$(SRCDIR)../dietlibc/include \
	  -I$(SRCDIR)../libpthread/include -I$(SRCDIR)../dietlibc/cpu/${CPU}

include $(SRCDIR)../lib.mk

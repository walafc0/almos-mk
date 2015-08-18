
LIB=	gomp

SRCS=	affinity.c alloc.c bar.c barrier.c critical.c \
	env.c error.c iter.c iter_ull.c lock.c \
	loop.c loop_ull.c mutex.c ordered.c  \
	parallel.c proc.c ptrlock.c sections.c sem.c \
	single.c task.c team.c time.c work.c

INCFLAGS= -I$(SRCDIR)/include -I$(SRCDIR)../dietlibc/include \
	  -I$(SRCDIR)../libpthread/include

include $(SRCDIR)../lib.mk

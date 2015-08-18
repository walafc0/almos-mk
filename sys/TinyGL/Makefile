
LIB=	TinyGL

VPATH= 	$(SRCDIR)src

SRCS=	api.c arrays.c clear.c clip.c error.c get.c image_util.c init.c \
	light.c list.c matrix.c memory.c misc.c msghandling.c oscontext.c \
	select.c specbuf.c texture.c vertex.c zbuffer.c zdither.c \
	zline.c zmath.c ztriangle.c

ifdef TINYGL_USE_GLX
SRCS+=	glx.c
endif
ifdef TINYGL_USE_NANOX
SRCS+=	nglx.c
endif

INCFLAGS= -I$(SRCDIR)/include -I$(SRCDIR)../dietlibc \
	  -I$(SRCDIR)../dietlibc/include -I$(SRCDIR)../libm/include


include $(SRCDIR)../lib.mk

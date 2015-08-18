
SUBDIR=	dietlibc libm libpthread libgomp libmemc \
	libz libcrypto libupc libphoenix TinyGL mk


default: all

$(SUBDIR)::
	@cd $@ && $(MAKE) $(MAKECMDGOALS)

all clean: $(SUBDIR)

.PHONY: all clean

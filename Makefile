
SUBDIR= kernel sys


default: all

$(SUBDIR)::
	@cd $@ && $(MAKE) $(MAKECMDGOALS)

all clean: $(SUBDIR)

.PHONY: all clean

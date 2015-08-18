## Note: $(ALMOS_USR_APPS) is defined in $ROOT_DIR/dev_env.sh
APPS=$(ALMOS_USR_APPS)

default: all

$(APPS)::
	@cd $@ && $(MAKE) $(MAKECMDGOALS)

all install clean realclean: $(APPS)

.PHONY: all install clean realclean


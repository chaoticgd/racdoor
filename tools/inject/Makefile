PROJECTDIR = ../..

BIN = inject
SRCS = inject.c $(PROJECTDIR)/sdk/crypto.c $(PROJECTDIR)/sdk/save.c

host: inject
ee:
clean: compileclean linkclean

include $(PROJECTDIR)/make/config_$(TARGET).mk
include $(PROJECTDIR)/make/compile.mk
include $(PROJECTDIR)/make/link_$(TARGET).mk

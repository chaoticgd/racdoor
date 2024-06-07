PROJECTDIR = ../..

BIN = rdxprep
SRCS = rdxprep.c $(PROJECTDIR)/sdk/linker.c

host: rdxprep
ee:
clean: compileclean linkclean

include $(PROJECTDIR)/make/config_$(TARGET).mk
include $(PROJECTDIR)/make/compile.mk
include $(PROJECTDIR)/make/link_$(TARGET).mk

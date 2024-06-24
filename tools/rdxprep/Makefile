PROJECTDIR = ../..

BIN = rdxprep
SRCS = rdxprep.c $(PROJECTDIR)/sdk/linker.c

host: rdxprep
ee:
clean: compileclean linkclean

include $(PROJECTDIR)/make/config.$(TARGET).mk
include $(PROJECTDIR)/make/compile.mk
include $(PROJECTDIR)/make/link.$(TARGET).mk

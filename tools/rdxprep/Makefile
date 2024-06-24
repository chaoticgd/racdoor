RACDOOR = ../..

BIN = rdxprep
SRCS = rdxprep.c $(RACDOOR)/sdk/linker.c

host: rdxprep
ee:
clean: compileclean linkclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/link.$(TARGET).mk

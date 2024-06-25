RACDOOR = ../..

BIN = tblgen
SRCS = tblgen.c $(RACDOOR)/sdk/elf.c

host: tblgen
ee:
clean: compileclean linkclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/link.$(TARGET).mk

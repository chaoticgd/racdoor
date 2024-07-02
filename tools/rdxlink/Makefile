RACDOOR = ../..

BIN = rdxlink
SRCS = rdxlink.c $(RACDOOR)/sdk/elf.c $(RACDOOR)/sdk/linker.c

host: rdxlink
ee:
clean: compileclean linkclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/link.$(TARGET).mk

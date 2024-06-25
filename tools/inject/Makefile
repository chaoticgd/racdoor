RACDOOR = ../..

BIN = inject
SRCS = inject.c compression.cpp $(RACDOOR)/sdk/crypto.c $(RACDOOR)/sdk/elf.c $(RACDOOR)/sdk/save.c

host: inject
ee:
clean: compileclean linkclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/link.$(TARGET).mk

# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

RACDOOR = ../..

BIN = sectiondump
SRCS = sectiondump.c $(RACDOOR)/sdk/buffer.c $(RACDOOR)/sdk/csv.c $(RACDOOR)/sdk/elf.c

host: sectiondump
ee:
clean: compileclean linkclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/link.$(TARGET).mk

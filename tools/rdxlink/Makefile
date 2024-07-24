# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

RACDOOR = ../..

BIN = rdxlink
SRCS = rdxlink.c $(RACDOOR)/sdk/buffer.c $(RACDOOR)/sdk/elf.c $(RACDOOR)/sdk/linker.c

host: rdxlink
ee:
clean: compileclean linkclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/link.$(TARGET).mk

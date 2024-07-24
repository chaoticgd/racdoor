# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

RACDOOR = ../..

BIN = inject
SRCS = inject.c compression.cpp $(RACDOOR)/sdk/buffer.c $(RACDOOR)/sdk/crypto.c $(RACDOOR)/sdk/elf.c $(RACDOOR)/sdk/exploit.c $(RACDOOR)/sdk/save.c

host: inject
ee:
clean: compileclean linkclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/link.$(TARGET).mk

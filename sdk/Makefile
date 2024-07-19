# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

RACDOOR := ..

LIB = sdk
SRCS = crypto.c elf.c exploit.c hook.c linker.c loader.s module.c persistence.c save.c

host: crypto.host.o elf.host.o exploit.host.o linker.host.o save.host.o
ee: sdk.ee.a loader.ee.o
clean: compileclean archiveclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/archive.mk

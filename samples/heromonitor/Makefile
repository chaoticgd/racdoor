# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

RACDOOR = ../..

BIN = heromonitor
SRCS = heromonitor.c $(RACDOOR)/extras/console.c
LIBS = $(RACDOOR)/sdk/sdk.$(TARGET).a

SERIALS = SCES-50916

host:
ee: heromonitor
clean: compileclean linkclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/link.$(TARGET).mk

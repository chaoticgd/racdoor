# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

# Toolchain
CC = ee-gcc
CXX = ee-g++
AR = ee-ar
OBJDUMP = ee-objdump

# Racdoor tools
CSVMERGE = $(RACDOOR)/tools/csvmerge/csvmerge
INJECT = $(RACDOOR)/tools/inject/inject
RDXLINK = $(RACDOOR)/tools/rdxlink/rdxlink
TBLGEN = $(RACDOOR)/tools/tblgen/tblgen

INCLUDES += -I$(RACDOOR)/include

COMPILEFLAGS += -std=c99 -Os -G0 -ffreestanding -mabi=eabi -mno-abicalls -fno-exceptions -Wall -Werror -D_EE

# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

# Toolchain
CC = gcc
CXX = g++
AR = ar
OBJDUMP = objdump

INCLUDES += -I$(RACDOOR)/include

COMPILEFLAGS += -std=c99 -O2 -Wall -Werror -D_HOST

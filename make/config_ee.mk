# Toolchain
CC = ee-gcc
CXX = ee-g++

# Racdoor tools
CSVMERGE = $(PROJECTDIR)/tools/csvmerge/csvmerge
RACDOOR = $(PROJECTDIR)/racdoor/racdoor
RDXGEN = $(PROJECTDIR)/tools/rdxgen/rdxgen
TBLGEN = $(PROJECTDIR)/tools/tblgen/tblgen

INCLUDES += -I$(PROJECTDIR)/include

COMPILEFLAGS += -std=c99 -Os -G0 -ffreestanding -nostdinc -mabi=eabi -mno-abicalls -fno-exceptions -D_EE

# Toolchain
CC = ee-gcc
CXX = ee-g++
AR = ee-ar

# Racdoor tools
CSVMERGE = $(PROJECTDIR)/tools/csvmerge/csvmerge
RACDOOR = $(PROJECTDIR)/racdoor/racdoor
RDXPREP = $(PROJECTDIR)/tools/rdxprep/rdxprep
TBLGEN = $(PROJECTDIR)/tools/tblgen/tblgen

INCLUDES += -I$(PROJECTDIR)/include

COMPILEFLAGS += -std=c99 -Os -G0 -ffreestanding -nostdinc -mabi=eabi -mno-abicalls -fno-exceptions -D_EE

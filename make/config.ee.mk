# Toolchain
CC = ee-gcc
CXX = ee-g++
AR = ee-ar
OBJDUMP = ee-objdump

# Racdoor tools
CSVMERGE = $(PROJECTDIR)/tools/csvmerge/csvmerge
INJECT = $(PROJECTDIR)/tools/inject/inject
RDXPREP = $(PROJECTDIR)/tools/rdxprep/rdxprep
TBLGEN = $(PROJECTDIR)/tools/tblgen/tblgen

INCLUDES += -I$(PROJECTDIR)/include

COMPILEFLAGS += -std=c99 -Os -G0 -ffreestanding -nostdinc -mabi=eabi -mno-abicalls -fno-exceptions -D_EE

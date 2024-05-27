# Toolchain
CC = ee-gcc
CXX = ee-g++

# Racdoor tools
RACDOOR = $(PROJECTDIR)/racdoor/racdoor
TBLGEN = $(PROJECTDIR)/tblgen/tblgen

INCLUDES += -I$(PROJECTDIR)/include

COMPILEFLAGS += -ffreestanding -nostdinc -mabi=eabi -mno-abicalls -fno-exceptions -D_EE
LINKFLAGS += -nostdlib -Wl,-T,$(PROJECTDIR)/linkfile.ld

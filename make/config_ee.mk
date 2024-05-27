# Toolchain
CC = ee-gcc
CXX = ee-g++

# Racdoor tools
ADDRGEN = $(PROJECTDIR)/addrgen/addrgen
RACDOOR = $(PROJECTDIR)/racdoor/racdoor

INCLUDES += -I$(PROJECTDIR)/include

COMPILEFLAGS += -ffreestanding -nostdinc -mabi=eabi -mno-abicalls -fno-exceptions -D_EE
LINKFLAGS += -nostdlib -Wl,-T,$(PROJECTDIR)/linkfile.ld

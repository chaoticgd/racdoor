RACDOOR := ..

LIB = sdk
SRCS = cleanup.c crypto.c elf.c hook.c linker.c loader.s module.c persistence.c save.c

host: crypto.host.o elf.host.o linker.host.o
ee: sdk.ee.a loader.ee.o
clean: compileclean archiveclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/archive.mk

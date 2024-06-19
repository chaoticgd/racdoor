PROJECTDIR := ..

LIB = sdk
SRCS = cleanup.c crypto.c hook.c linker.c loader.s save.c

host: crypto.host.o linker.host.o
ee: sdk.ee.a loader.ee.o
clean: compileclean archiveclean

include $(PROJECTDIR)/make/config_$(TARGET).mk
include $(PROJECTDIR)/make/compile.mk
include $(PROJECTDIR)/make/archive.mk

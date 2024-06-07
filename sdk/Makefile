PROJECTDIR := ..

LIB = sdk
SRCS = linker.c hook.c

host: linker.host.o
ee: sdk.ee.a
clean: compileclean archiveclean

include $(PROJECTDIR)/make/config_$(TARGET).mk
include $(PROJECTDIR)/make/compile.mk
include $(PROJECTDIR)/make/archive.mk

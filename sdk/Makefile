PROJECTDIR := ..

SRCS = linker.c hook.c

host: linker.host.o
ee: linker.ee.o hook.ee.o
clean: compileclean linkclean

include $(PROJECTDIR)/make/config_$(TARGET).mk
include $(PROJECTDIR)/make/compile.mk
include $(PROJECTDIR)/make/link_$(TARGET).mk

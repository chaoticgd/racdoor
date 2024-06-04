PROJECTDIR = ../..

BIN = hello
SRCS = hello.cpp $(PROJECTDIR)/sdk/linker.c

SERIALS = SCES-53285

host:
ee: hello
clean: compileclean linkclean

include $(PROJECTDIR)/make/config_$(TARGET).mk
include $(PROJECTDIR)/make/compile.mk
include $(PROJECTDIR)/make/link_$(TARGET).mk

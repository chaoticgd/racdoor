PROJECTDIR = ../..

BIN = hello
SRCS = hello.cpp
LIBS = $(PROJECTDIR)/sdk/sdk.$(TARGET).a

SERIALS = SCES-53285

host:
ee: hello
clean: compileclean linkclean

include $(PROJECTDIR)/make/config_$(TARGET).mk
include $(PROJECTDIR)/make/compile.mk
include $(PROJECTDIR)/make/link_$(TARGET).mk

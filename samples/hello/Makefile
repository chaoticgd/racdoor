RACDOOR = ../..

BIN = hello
SRCS = hello.c
LIBS = $(RACDOOR)/sdk/sdk.$(TARGET).a

SERIALS = SCES-50916

host:
ee: hello
clean: compileclean linkclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/link.$(TARGET).mk

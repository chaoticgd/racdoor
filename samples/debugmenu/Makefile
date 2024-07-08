RACDOOR = ../..

BIN = debugmenu
SRCS = debugmenu.c
LIBS = $(RACDOOR)/sdk/sdk.$(TARGET).a

SERIALS = SCES-50916 SCUS-97199

host:
ee: debugmenu
clean: compileclean linkclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/link.$(TARGET).mk

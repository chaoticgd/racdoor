RACDOOR = ../..

BIN = heromonitor
SRCS = heromonitor.c $(RACDOOR)/extras/console.c
LIBS = $(RACDOOR)/sdk/sdk.$(TARGET).a

SERIALS = SCES-50916

host:
ee: heromonitor
clean: compileclean linkclean

include $(RACDOOR)/make/config.$(TARGET).mk
include $(RACDOOR)/make/compile.mk
include $(RACDOOR)/make/link.$(TARGET).mk

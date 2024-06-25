OBJS := $(patsubst %,%.$(TARGET).o,$(basename $(SRCS)))
DEPS := $(patsubst %,%.$(TARGET).d,$(basename $(SRCS)))

# Generate dependency files during compilation. These are makefiles that contain
# rules for building the individual objects file that specify all the header
# files they include as prerequisites.
DEPFLAGS = -MT $@ -MMD -MP -MF $*.$(TARGET).d

%.$(TARGET).o: %.s
%.$(TARGET).o: %.s %.$(TARGET).d
	$(CC) -c $< -o $@ $(DEPFLAGS) $(COMPILEFLAGS) $(INCLUDES)

%.$(TARGET).o: %.c
%.$(TARGET).o: %.c %.$(TARGET).d
	$(CC) -c $< -o $@ $(DEPFLAGS) $(COMPILEFLAGS) $(INCLUDES)

%.$(TARGET).o: %.cpp
%.$(TARGET).o: %.cpp %.$(TARGET).d
	$(CC) -c $< -o $@ $(DEPFLAGS) $(INCLUDES)

$(DEPS):

compileclean:
	$(RM) $(OBJS) $(DEPS)

include $(wildcard $(DEPS))

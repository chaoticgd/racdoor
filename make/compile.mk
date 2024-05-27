OBJS := $(patsubst %,%.o,$(basename $(SRCS)))
DEPS := $(patsubst %,%.d,$(basename $(SRCS)))

# Generate dependency files during compilation. These are makefiles that contain
# rules for building the individual objects file that specify all the header
# files they include as prerequisites.
DEPFLAGS = -MT $@ -MMD -MP -MF $*.d

%.o: %.c
%.o: %.c %.d
	$(CC) -c $(DEPFLAGS) $(CFLAGS) $(INCLUDES) $<

%.o: %.cpp
%.o: %.cpp %.d
	$(CXX) -c $(DEPFLAGS) $(CFLAGS) $(INCLUDES) $<

$(DEPS):

compileclean:
	$(RM) $(OBJS) $(DEPS)

include $(wildcard $(DEPS))

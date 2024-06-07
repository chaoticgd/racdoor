$(LIB).$(TARGET).a: $(OBJS)
	$(AR) r $@ $(OBJS)

archiveclean:
	$(RM) $(LIB).$(TARGET).a

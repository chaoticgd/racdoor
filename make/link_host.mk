$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LINKFLAGS)
	
linkclean:
	$(RM) $(BIN)

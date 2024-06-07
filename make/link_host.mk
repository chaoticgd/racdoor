$(BIN): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $@
	
linkclean:
	$(RM) $(BIN)

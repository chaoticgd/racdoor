$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@
	
linkclean:
	$(RM) $(BIN)

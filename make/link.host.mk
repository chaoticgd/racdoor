$(BIN): $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $@
	
linkclean:
	$(RM) $(BIN)

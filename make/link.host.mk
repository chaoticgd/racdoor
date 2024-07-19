# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

$(BIN): $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $@
	
linkclean:
	$(RM) $(BIN)

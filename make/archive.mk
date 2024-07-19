# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

$(LIB).$(TARGET).a: $(OBJS)
	$(AR) r $@ $(OBJS)

archiveclean:
	$(RM) $(LIB).$(TARGET).a

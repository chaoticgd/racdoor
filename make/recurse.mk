# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

# Enumerate all subdirectories.
SUBDIRS = $(dir $(wildcard */Makefile))

all: host ee

# Run 'make host' on each subdirectory.
HOST_SUBDIRS = $(addprefix host-,$(SUBDIRS))
.PHONY: host $(HOST_SUBDIRS)
host: $(HOST_SUBDIRS)
$(HOST_SUBDIRS):
	TARGET=host $(MAKE) -C $(patsubst host-%,%,$@) host

# Run 'make ee' on each subdirectory.
EE_SUBDIRS = $(addprefix ee-,$(SUBDIRS))
.PHONY: ee $(EE_SUBDIRS)
ee: $(EE_SUBDIRS)
$(EE_SUBDIRS): host
	TARGET=ee $(MAKE) -C $(patsubst ee-%,%,$@) ee

# Run 'make clean' on each subdirectory.
CLEAN_SUBDIRS = $(addprefix clean-,$(SUBDIRS))
.PHONY: clean $(CLEAN_SUBDIRS)
clean: $(CLEAN_SUBDIRS)
$(CLEAN_SUBDIRS):
	$(MAKE) -C $(patsubst clean-%,%,$@) clean

# Disable implicit rules.
MAKEFLAGS += -r

SUBDIRS = sdk extras tools tables samples

all: host ee

# Run 'make host' on each subdirectory in order.
HOST_SUBDIRS = $(addprefix host-,$(SUBDIRS))
.PHONY: host $(HOST_SUBDIRS)
host: $(HOST_SUBDIRS)
$(HOST_SUBDIRS):
	TARGET=host $(MAKE) -C $(patsubst host-%,%,$@) host

# Run 'make ee' on each subdirectory in order.
EE_SUBDIRS = $(addprefix ee-,$(SUBDIRS))
.PHONY: ee $(EE_SUBDIRS)
ee: $(EE_SUBDIRS)
$(EE_SUBDIRS): host
	TARGET=ee $(MAKE) -C $(patsubst ee-%,%,$@) ee

.PHONY: doc
doc:
	$(MAKE) -C doc

# Run 'make clean' on each subdirectory in order.
CLEAN_SUBDIRS = $(addprefix clean-,$(SUBDIRS))
.PHONY: clean $(CLEAN_SUBDIRS)
clean: $(CLEAN_SUBDIRS)
$(CLEAN_SUBDIRS):
	TARGET=host $(MAKE) -C $(patsubst clean-%,%,$@) clean
	TARGET=ee $(MAKE) -C $(patsubst clean-%,%,$@) clean

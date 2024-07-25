# This file is part of Racdoor.
# Copyright (c) 2024 chaoticgd. All rights reversed.
# Released under the BSD-1-Clause license.

TBLS := $(patsubst %,$(BIN).%.tbl,$(basename $(SERIALS)))
ELFS := $(patsubst %,$(BIN).%.elf,$(basename $(SERIALS)))
RDXS := $(patsubst %,$(BIN).%.rdx,$(basename $(SERIALS)))
MAPS := $(patsubst %,$(BIN).%.map,$(basename $(SERIALS)))
DISS := $(patsubst %,$(BIN).%.dis,$(basename $(SERIALS)))

# Generate an object file containing .symtab symbols for all the core symbols
# loaded from the provided CSV file and some custom sections for all of the
# overlay symbols.
.PRECIOUS: $(BIN).%.tbl
$(BIN).%.tbl: $(TBLGEN) $(OBJS) $(LIBS) $(RACDOOR)/tables/output/%.csv
	$(TBLGEN) $(OBJS) $(LIBS) -t $(RACDOOR)/tables/output/$*.csv -s $* -o $@

# Perform partial linking with the -r option passed to GNU ld. This will combine
# together the sections from the object files given as input but on its own will
# not apply relocations for external symbols. The rdxlink step that runs right
# after will apply the static relocations, and will prepare the dynamic
# relocations for application at runtime.
.PRECIOUS: $(BIN).%.elf
$(BIN).%.elf: $(RDXLINK) $(RACDOOR)/sdk/loader.ee.o $(OBJS) $(LIBS) $(BIN).%.tbl $(RACDOOR)/linkfile.ld
	$(CC) $(RACDOOR)/sdk/loader.ee.o $(OBJS) $(LIBS) $(BIN).$*.tbl -o $@ -nostdlib -Wl,-r,-T,$(RACDOOR)/linkfile.ld,-Map,$(BIN).$*.map
	$(RDXLINK) $(BIN).$*.elf $(BIN).$*.elf

# Pack the sections from the ELF into a custom compact container format that is
# to be used at runtime.
$(BIN).%.rdx: $(RDXPACK) $(BIN).%.elf
	$(RDXPACK) $(BIN).$*.elf $@

# Automatically generate disassemblies for all the RDX files.
$(BIN).%.dis: $(BIN).%.elf
	$(OBJDUMP) -D $(BIN).$*.elf > $@

.PHONY: $(BIN)
$(BIN): $(RDXS) $(DISS)

linkclean:
	$(RM) $(TBLS) $(ELFS) $(RDXS) $(DISS) $(MAPS)

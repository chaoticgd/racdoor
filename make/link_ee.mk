TBLS := $(patsubst %,$(BIN).%.tbl,$(basename $(SERIALS)))
ELFS := $(patsubst %,$(BIN).%.elf,$(basename $(SERIALS)))
RDXS := $(patsubst %,$(BIN).%.rdx,$(basename $(SERIALS)))
MAPS := $(patsubst %,$(BIN).%.map,$(basename $(SERIALS)))

SERIAL_FROM_TABLE = $(patsubst $(BIN).%.tbl,%,$@)
SERIAL_FROM_BIN = $(patsubst $(BIN).%.elf,%,$@)
TABLE_FROM_BIN = $(patsubst $(BIN).%.elf,$(BIN).%.tbl,$@)
BIN_FROM_RDX = $(patsubst $(BIN).%.rdx,$(BIN).%.elf,$@)

# Generate an object file containing .symtab symbols for all the core symbols
# loaded from the provided CSV file and some custom sections for all of the
# overlay symbols.
.PRECIOUS: $(BIN).%.tbl
$(BIN).%.tbl: $(TBLGEN) $(OBJS) $(PROJECTDIR)/tables/%.csv
	$(TBLGEN) $(OBJS) -t $(PROJECTDIR)/tables/$(SERIAL_FROM_TABLE).csv -o $@ -v

# Perform partial linking with the -r option passed to GNU ld. This will combine
# together the sections from the object files given as input but it will not
# apply relocations for external symbols.
.PRECIOUS: $(BIN).%.elf
$(BIN).%.elf: $(OBJS) $(BIN).%.tbl $(PROJECTDIR)/linkfile.ld
	$(CC) $(OBJS) $(TABLE_FROM_BIN) -o $@ -nostdlib -Wl,-r,-T,$(PROJECTDIR)/linkfile.ld,-Map,$(BIN).$(SERIAL_FROM_BIN).map

# Apply relocations for static symbols and copy relocations for dynamic symbols
# into the .racdoor.relocs section so that they can be applied at runtime.
$(BIN).%.rdx: $(RDXGEN) $(BIN).%.elf
	$(RDXGEN) $(BIN_FROM_RDX) $@

.PHONY: $(BIN)
$(BIN): $(RDXS)

linkclean:
	$(RM) $(TBLS) $(ELFS) $(RDXS) $(MAPS)

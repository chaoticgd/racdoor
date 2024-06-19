TBLS := $(patsubst %,$(BIN).%.tbl,$(basename $(SERIALS)))
ELFS := $(patsubst %,$(BIN).%.elf,$(basename $(SERIALS)))
RDXS := $(patsubst %,$(BIN).%.rdx,$(basename $(SERIALS)))
MAPS := $(patsubst %,$(BIN).%.map,$(basename $(SERIALS)))

# Generate an object file containing .symtab symbols for all the core symbols
# loaded from the provided CSV file and some custom sections for all of the
# overlay symbols.
.PRECIOUS: $(BIN).%.tbl
$(BIN).%.tbl: $(TBLGEN) $(OBJS) $(PROJECTDIR)/tables/output/%.csv
	$(TBLGEN) $(OBJS) $(LIBS) -t $(PROJECTDIR)/tables/output/$*.csv -s $* -o $@ -v

# Perform partial linking with the -r option passed to GNU ld. This will combine
# together the sections from the object files given as input but it will not
# apply relocations for external symbols.
.PRECIOUS: $(BIN).%.elf
$(BIN).%.elf: $(PROJECTDIR)/sdk/loader.ee.o $(OBJS) $(LIBS) $(BIN).%.tbl $(PROJECTDIR)/linkfile.ld
	$(CC) $(PROJECTDIR)/sdk/loader.ee.o $(OBJS) $(LIBS) $(BIN).$*.tbl -o $@ -nostdlib -Wl,-r,-T,$(PROJECTDIR)/linkfile.ld,-Map,$(BIN).$*.map

# Apply relocations for static symbols and copy relocations for dynamic symbols
# into the .racdoor.relocs section so that they can be applied at runtime.
$(BIN).%.rdx: $(RDXPREP) $(BIN).%.elf
	$(RDXPREP) $(BIN).$*.elf $@

.PHONY: $(BIN)
$(BIN): $(RDXS)

linkclean:
	$(RM) $(TBLS) $(ELFS) $(RDXS) $(MAPS)

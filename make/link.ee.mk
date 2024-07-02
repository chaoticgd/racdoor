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
	$(TBLGEN) $(OBJS) $(LIBS) -t $(RACDOOR)/tables/output/$*.csv -s $* -o $@ -v

# Perform partial linking with the -r option passed to GNU ld. This will combine
# together the sections from the object files given as input but it will not
# apply relocations for external symbols.
.PRECIOUS: $(BIN).%.elf
$(BIN).%.elf: $(RACDOOR)/sdk/loader.ee.o $(OBJS) $(LIBS) $(BIN).%.tbl $(RACDOOR)/linkfile.ld
	$(CC) $(RACDOOR)/sdk/loader.ee.o $(OBJS) $(LIBS) $(BIN).$*.tbl -o $@ -nostdlib -Wl,-r,-T,$(RACDOOR)/linkfile.ld,-Map,$(BIN).$*.map

# Apply relocations for static symbols and copy relocations for dynamic symbols
# into the .racdoor.relocs section so that they can be applied at runtime.
$(BIN).%.rdx: $(RDXPREP) $(BIN).%.elf
	$(RDXPREP) $(BIN).$*.elf $@

# Automatically generate disassemblies for all the RDX files.
$(BIN).%.dis: $(BIN).%.rdx
	$(OBJDUMP) -D $(BIN).$*.rdx > $@

.PHONY: $(BIN)
$(BIN): $(RDXS) $(DISS)

linkclean:
	$(RM) $(TBLS) $(ELFS) $(RDXS) $(DISS) $(MAPS)

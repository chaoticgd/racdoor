TABLES := $(patsubst %,$(BIN).%.t,$(basename $(SERIALS)))
BINS := $(patsubst %,$(BIN).%.elf,$(basename $(SERIALS)))

SERIAL_FROM_TABLE = $(patsubst $(BIN).%.t,%,$@)
TABLE_FROM_BIN = $(patsubst $(BIN).%.elf,$(BIN).%.t,$@)

.PRECIOUS: $(BIN).%.t
$(BIN).%.t: $(OBJS) $(PROJECTDIR)/tables/%.csv $(TBLGEN)
	$(TBLGEN) $(OBJS) -t $(PROJECTDIR)/tables/$(SERIAL_FROM_TABLE).csv -o $@ -v

$(BIN).%.elf: $(OBJS) $(BIN).%.t $(PROJECTDIR)/linkfile.ld
	$(CC) $(OBJS) $(TABLE_FROM_BIN) -o $@ $(LINKFLAGS)

.PHONY: $(BIN)
$(BIN): $(BINS)

linkclean:
	$(RM) $(TABLES) $(BINS)

PROJECTDIR = ..

# Enumerate the input CSV files. These will all have filenames that conform to
# the following format: "$(CATEGORY).$(SERIAL).csv".
CATEGORY_DOT_SERIAL_DOT_CSV = $(wildcard *.csv)
CATEGORY_DOT_SERIAL = $(patsubst %.csv,%,$(CATEGORY_DOT_SERIAL_DOT_CSV))
CATEGORY_SPACE_SENTINEL_SERIAL = $(subst ., __serial__,$(CATEGORY_DOT_SERIAL))

CATEGORIES = $(sort $(filter-out __serial__%,$(CATEGORY_SPACE_SENTINEL_SERIAL)))
SERIALS = $(sort $(patsubst __serial__%,%,$(filter __serial__%,$(CATEGORY_SPACE_SENTINEL_SERIAL))))

OUTPUTS = $(patsubst %,output/%.csv,$(SERIALS))

$(shell mkdir -p output)

host:
ee: $(OUTPUTS)
clean:
	$(RM) -r output

.SECONDEXPANSION:
output/%.csv: $(CSVMERGE) $$(wildcard *.%.csv)
	$(CSVMERGE) $(wildcard *.$*.csv) -o $@

include $(PROJECTDIR)/make/config_$(TARGET).mk

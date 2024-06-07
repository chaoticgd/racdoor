#include <racdoor/buffer.h>
#include <racdoor/elf.h>
#include <racdoor/util.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MAX_OVERLAYS 100

typedef struct {
	const char* name;
	const char* comment;
	ElfSymbolType type;
	u32 size;
	u32 core_address;
	u32 spcore_address;
	u32 mpcore_address;
	u32 frontend_address;
	u32 frontbin_address;
	u32 overlay_addresses[MAX_OVERLAYS];
	s32 runtime_index;
	s8 overlay;
	s8 used;
} Symbol;

#define MAX_LEVELS 100

typedef struct {
	u32 overlay_count;
	u8 levels[MAX_LEVELS];
	u32 level_count;
	Symbol* symbols;
	u32 symbol_count;
} SymbolTable;

typedef enum {
	COLUMN_NAME, /* Symbol name. This should be a non-mangled C identifier. */
	COLUMN_TYPE, /* ELF symbol type. */
	COLUMN_COMMENT, /* Ignored. */
	COLUMN_SIZE, /* Size in bytes. */
	COLUMN_CORE, /* Address in a core section from the ELF. These are always loaded. */
	COLUMN_SPCORE, /* UYA specific. Address in a core section from the singleplayer ELF. */
	COLUMN_MPCORE, /* UYA specific. Address in a core section from the multiplayer ELF. */
	COLUMN_FRONTEND, /* Address in the main menu overlay from the ELF. */
	COLUMN_FRONTBIN, /* Address in the main menu overlay from MISC.WAD. */
	COLUMN_FIRST_OVERLAY, /* Address in a level overlay from the LEVEL*.WAD files. */
	MAX_COLUMNS = 100
} Column;

typedef struct {
	u32 column;
	const char* string;
} ColumnName;

ColumnName column_names[] = {
	{COLUMN_NAME    , "NAME"    },
	{COLUMN_TYPE    , "TYPE"    },
	{COLUMN_COMMENT , "COMMENT" },
	{COLUMN_SIZE    , "SIZE"    },
	{COLUMN_CORE    , "CORE"    },
	{COLUMN_SPCORE  , "SPCORE"  },
	{COLUMN_MPCORE  , "MPCORE"  },
	{COLUMN_FRONTEND, "FRONTEND"},
	{COLUMN_FRONTBIN, "FRONTBIN"}
};

static SymbolTable parse_table(Buffer input);
static u32 parse_table_header(const char** p, SymbolTable* table, Column* columns);
static u32 parse_object_file(SymbolTable* table, Buffer object);
static void map_symbols_to_runtime_indices(SymbolTable* table);
static void print_table(SymbolTable* table);
static Buffer build_object_file(SymbolTable* table, u32 relocation_count, const char* serial);
static int symbolmap_comparator(const void* lhs, const void* rhs);
static u32 find_string(const char* string, const char** array, u32 count);

static char* string_section; /* Used by symbolmap_comparator. */

int main(int argc, char** argv)
{
	Buffer* input_objects = checked_malloc(argc * sizeof(Buffer));
	u32 input_object_count = 0;
	Buffer input_table = {};
	const char* serial = NULL;
	const char* output_object_path = NULL;
	int verbose = 0;
	
	/* Parse the command line arguments. */
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-t") == 0)
			input_table = read_file(argv[++i]);
		else if (strcmp(argv[i], "-s") == 0)
			serial = argv[++i];
		else if (strcmp(argv[i], "-o") == 0)
			output_object_path = argv[++i];
		else if (strcmp(argv[i], "-v") == 0)
			verbose = 1;
		else
			input_objects[input_object_count++] = read_file(argv[i]);
	}
	
	CHECK(input_object_count && input_table.data && serial && output_object_path,
		"usage: %s <input objects...> -t <input table> -s <serial> -o <output object> [-v]\n",
		(argc > 0) ? argv[0] : "tblgen");
	
	/* Parse the input CSV file into a symbol table in memory. */
	SymbolTable table = parse_table(input_table);
	
	/* Check which symbols are actually referenced by the object files being
	   built so we can omit the rest of them from the build later. */
	u32 relocation_count = 0;
	for (u32 i = 0; i < input_object_count; i++)
		relocation_count += parse_object_file(&table, input_objects[i]);
	
	/* Determine where each symbol lives in the runtime address table and count
	   the number of symbols to be included. */
	map_symbols_to_runtime_indices(&table);
	
	/* Print out the contents of the symbol table. */
	if (verbose)
		print_table(&table);
	
	/* Build an object file containing the symbols from the CSV file. */
	Buffer elf = build_object_file(&table, relocation_count, serial);
	
	/* Write out the new object file. */
	write_file(output_object_path, elf);
	
	return 0;
}

static SymbolTable parse_table(Buffer input)
{
	SymbolTable table = {};
	memset(table.levels, 0xff, sizeof(table.levels));
	
	Column columns[MAX_COLUMNS];
	
	const char* ptr = input.data;
	
	/* First we iterate over all of the headings and record information about
	   what is stored in each of the columns, and how each of the levels relate
	   to the overlays. */
	u32 column_count = parse_table_header(&ptr, &table, columns);
	
	/* Each remaining line in the table file will be a symbol. */
	const char* backup_ptr = ptr;
	while (ptr < input.data + input.size)
		if (*ptr++ == '\n')
			table.symbol_count++;
	ptr = backup_ptr;
	
	table.symbols = checked_malloc(table.symbol_count * sizeof(Symbol));
	memset(table.symbols, 0, table.symbol_count * sizeof(Symbol));
	
	u32 symbol = 0;
	u32 column = 0;
	
	/* Finally we iterate over each of the symbols and fill in all the names,
	   sizes, and addresses from the rows in the CSV file. */
	while (ptr < input.data + input.size)
	{
		if (*(ptr - 1) == '\n')
		{
			column = 0;
			
			while (*ptr != '\n' && ptr < input.data + input.size)
			{
				CHECK(column < column_count, "Mismatched columns on line %d.\n", symbol + 1);
				
				switch (columns[column])
				{
					case COLUMN_NAME:
					case COLUMN_COMMENT:
					{
						const char* begin;
						const char* end;
						if (*ptr == '"')
						{
							begin = ptr + 1;
							end = strstr(begin, "\"");
						}
						else
						{
							begin = ptr;
							end = strstr(begin, ",");
						}
						
						if (!end)
							end = strstr(ptr, "\n");
						CHECK(end, "Unexpected end of input table file on line %d.\n", symbol + 1);
						
						char* string = checked_malloc(end - begin + 1);
						memcpy(string, begin, end - begin);
						string[end - begin] = '\0';
						
						if (columns[column] == COLUMN_NAME)
							table.symbols[symbol].name = string;
						else
							table.symbols[symbol].comment = string;
						
						ptr = end;
						if (*ptr == '"')
							ptr++;
						
						break;
					}
					case COLUMN_TYPE:
						if (strncmp(ptr, "NOTYPE", 6) == 0)
							table.symbols[symbol].type = STT_NOTYPE;
						else if (strncmp(ptr, "OBJECT", 6) == 0)
							table.symbols[symbol].type = STT_OBJECT;
						else if (strncmp(ptr, "FUNC", 4) == 0)
							table.symbols[symbol].type = STT_FUNC;
						else if (strncmp(ptr, "SECTION", 7) == 0)
							table.symbols[symbol].type = STT_SECTION;
						else if (strncmp(ptr, "FILE", 4) == 0)
							table.symbols[symbol].type = STT_FILE;
						else if (strncmp(ptr, "COMMON", 6) == 0)
							table.symbols[symbol].type = STT_COMMON;
						else if (strncmp(ptr, "TLS", 3) == 0)
							table.symbols[symbol].type = STT_TLS;
						else
							ERROR("Invalid TYPE on line %d.\n", symbol + 1);
						
						while (*ptr >= 'A' && *ptr <= 'Z')
							ptr++;
						
						break;
					default:
					{
						if (*ptr == ',')
							break;
						
						char* end = NULL;
						u32 value = (u32) strtoul(ptr, &end, 16);
						CHECK(end != ptr, "Invalid %s on line %d.\n",
							columns[column] == COLUMN_SIZE ? "SIZE" : "address",
							symbol + 1);
						
						if (columns[column] == COLUMN_SIZE)
							table.symbols[symbol].size = value;
						else if (columns[column] == COLUMN_CORE)
							table.symbols[symbol].core_address = value;
						else if (columns[column] == COLUMN_SPCORE)
							table.symbols[symbol].spcore_address = value;
						else if (columns[column] == COLUMN_MPCORE)
							table.symbols[symbol].mpcore_address = value;
						else if (columns[column] == COLUMN_FRONTEND)
							table.symbols[symbol].frontend_address = value;
						else if (columns[column] == COLUMN_FRONTBIN)
							table.symbols[symbol].frontbin_address = value;
						else
						{
							u32 overlay = columns[column] - COLUMN_FIRST_OVERLAY;
							CHECK(overlay < table.overlay_count, "Invalid column on line %d.\n", symbol + 1);
							
							table.symbols[symbol].overlay_addresses[overlay] = value;
							table.symbols[symbol].overlay = 1;
							
							CHECK(table.symbols[symbol].core_address == 0,
								"Symbol '%s' has multiple addresses in the CSV file.\n",
								table.symbols[symbol].name);
						}
						
						ptr = end;
					}
				}
				
				CHECK(*ptr == ',' || *ptr == '\n', "Invalid delimiters on line %d.\n", symbol + 1);
				if (*ptr == ',')
					ptr++;
				
				column++;
			}
			
			symbol++;
		}
		else
			ptr++;
	}
	
	return table;
}

static u32 parse_table_header(const char** p, SymbolTable* table, Column* columns)
{
	const char* ptr = *p;
	u32 column = 0;
	
	while (*ptr != '\0' && *ptr != '\n')
	{
		CHECK(column < MAX_COLUMNS, "Too many columns.\n");
		
		int quoted = 0;
		if (*ptr == '"')
		{
			quoted = 1;
			ptr++;
		}
		
		int done = 0;
		for (u32 i = 0; i < ARRAY_SIZE(column_names); i++)
		{
			if (strncmp(ptr, column_names[i].string, strlen(column_names[i].string)) == 0)
			{
				columns[column] = column_names[i].column;
				ptr += strlen(column_names[i].string);
				done = 1;
				break;
			}
		}
		
		if (!done)
		{
			columns[column] = COLUMN_FIRST_OVERLAY + table->overlay_count;
			
			char* end = NULL;
			u32 first, last;
			char opening = *ptr;
			
			switch (opening)
			{
				case '[': /* Closed interval. */
					ptr++;
					
					first = strtoul(ptr, &end, 10);
					CHECK(end != ptr && first < MAX_LEVELS, "Invalid interval column heading (bad level number).\n");
					ptr = end;
					
					CHECK(*ptr++ == ',', "Invalid interval column heading (missing comma).\n");
					
					last = strtoul(ptr, &end, 10);
					CHECK(end != ptr && last < MAX_LEVELS && first < last, "Invalid interval column heading (bad level number).\n");
					ptr = end;
					
					CHECK(*ptr++ == ']', "Invalid interval column heading (missing closing bracket).");
					
					for (u32 i = first; i <= last; i++)
						table->levels[i] = (u8) table->overlay_count;
					
					table->level_count = MAX(table->level_count, last);
					
					break;
				case '{': /* Set. */
					do
					{
						ptr++;
						
						u32 level = strtoul(ptr, &end, 10);
						CHECK(end != ptr && level < MAX_LEVELS, "Invalid set column heading (bad level number).\n");
						ptr = end;
						
						table->levels[level] = (u8) table->overlay_count;
						table->level_count = MAX(table->level_count, level);
					} while (*ptr == ',');
					
					CHECK(*ptr++ == '}', "Invalid set column heading (missing closing bracket).\n");
					
					break;
				default: /* Scalar. */
					first = strtoul(ptr, &end, 10);
					CHECK(end != ptr && first < MAX_LEVELS, "Invalid scalar column heading (bad level number).\n");
					ptr = end;
					
					table->levels[first] = (u8) table->overlay_count;
					table->level_count = MAX(table->level_count, first + 1);
			}
			
			table->overlay_count++;
		}
		
		if (quoted)
			CHECK(*ptr++ == '"', "Unexpected characters in table header.\n");
		
		if (*ptr == ',')
			ptr++;
		
		column++;
	}
	
	CHECK(*ptr++ == '\n', "Unexpected end of input table file.\n");
	
	*p = ptr;
	
	return column;
}

static u32 parse_object_file(SymbolTable* table, Buffer object)
{
	ElfFileHeader* header = buffer_get(object, 0, sizeof(ElfFileHeader), "ELF header");
	u32 shstrtab_offset = header->shoff + header->shstrndx * sizeof(ElfSectionHeader);
	ElfSectionHeader* shstrtab = buffer_get(object, shstrtab_offset, sizeof(ElfSectionHeader), "section header");
	
	/* Find the symbol table section. */
	ElfSectionHeader* symtab = NULL;
	for (u32 i = 0; i < header->shnum; i++)
	{
		u32 section_offset = header->shoff + i * sizeof(ElfSectionHeader);
		ElfSectionHeader* section = buffer_get(object, section_offset, sizeof(ElfSectionHeader), "section header");
		
		const char* name = buffer_string(object, shstrtab->offset + section->name, "section name");
		if (strcmp(name, ".symtab") == 0)
			symtab = section;
	}
	
	CHECK(symtab, "No .symtab section.\n");
	
	/* Find the string table section. */
	ElfSectionHeader* strtab = NULL;
	if (symtab->link != 0)
	{
		u32 link_offset = header->shoff + symtab->link * sizeof(ElfSectionHeader);
		strtab = buffer_get(object, link_offset, sizeof(ElfSectionHeader), "linked section header");
	}
	
	CHECK(strtab, "No linked string table section.\n");
	
	/* Mark symbols that are referenced in the input object file as used. */
	ElfSymbol* symbols = buffer_get(object, symtab->offset, symtab->size, "symbol table");
	for (u32 i = 0; i < symtab->size / sizeof(ElfSymbol); i++)
	{
		const char* name = buffer_string(object, strtab->offset + symbols[i].name, "symbol name");
		if (strlen(name) == 0)
			continue;
		
		for (u32 j = 0; j < table->symbol_count; j++)
			if (table->symbols[j].name && strcmp(table->symbols[j].name, name) == 0)
				table->symbols[j].used = 1;
	}
	
	u32 relocation_count = 0;
	
	/* Count the number of relocations that will need to be available at runtime
	   so that we can set create a placeholder section for them. */
	for (u32 i = 0; i < header->shnum; i++)
	{
		u32 section_offset = header->shoff + i * sizeof(ElfSectionHeader);
		ElfSectionHeader* section = buffer_get(object, section_offset, sizeof(ElfSectionHeader), "section header");
		if (section->type != SHT_REL)
			continue;
		
		ElfRelocation* relocs = buffer_get(object, section->offset, section->size, "relocations");
		for (u32 j = 0; j < section->size / sizeof(ElfRelocation); j++)
		{
			unsigned int symbol_index = relocs[j].info >> 8;
			CHECK(symbol_index < symtab->size / sizeof(ElfSymbol), "Invalid symbol index %u.\n", symbol_index);
			ElfSymbol* symbol = &symbols[symbol_index];
			
			const char* name = buffer_string(object, strtab->offset + symbol->name, "symbol name");
			if (strlen(name) == 0)
				continue;
			
			for (u32 k = 0; k < table->symbol_count; k++)
			{
				if (table->symbols[k].overlay && table->symbols[k].name && strcmp(table->symbols[k].name, name) == 0)
				{
					relocation_count++;
					break;
				}
			}
		}
	}
	
	return relocation_count;
}

static void map_symbols_to_runtime_indices(SymbolTable* table)
{
	s32 next_index = 0;
	for (u32 i = 0; i < table->symbol_count; i++)
	{
		if (table->symbols[i].used && table->symbols[i].overlay)
		{
			table->symbols[i].runtime_index = next_index++;
			continue;
		}
		table->symbols[i].runtime_index = -1;
	}
}

static void print_table(SymbolTable* table)
{
	printf("%d overlays:\n", table->overlay_count);
	for(u32 i = 0; i < table->overlay_count; i++)
	{
		printf("  %d: ", i);
		int printed = 0;
		for(u32 j = 0; j < table->level_count; j++)
			if (table->levels[j] == i)
				printed = printf("%s%d", printed ? ", " : "", j);
		printf("\n");
	}
	
	printf("\n");
	printf("%d symbols:\n", table->symbol_count);
	for (u32 i = 0; i < table->symbol_count; i++)
	{
		int last = 0;
		
		printf("  %s%s", table->symbols[i].name ? table->symbols[i].name : "(null)", table->symbols[i].used ? "*" : "");
		if (table->symbols[i].runtime_index > -1)
			printf(" [%d]", table->symbols[i].runtime_index);
		printf(" = {");
		if (table->symbols[i].size != 0)
			last = printf("%s size = %x", last ? "," : "", table->symbols[i].size);
		if (table->symbols[i].core_address != 0)
			last = printf("%s core = %x", last ? "," : "", table->symbols[i].core_address);
		for (u32 j = 0; j < table->overlay_count; j++)
			if (table->symbols[i].overlay_addresses[j])
				last = printf("%s overlay[%d] = %x", last ? "," : "", j, table->symbols[i].overlay_addresses[j]);
		printf(" }\n");
	}
}

static Buffer build_object_file(SymbolTable* table, u32 relocation_count, const char* serial)
{
	/* Count the number of symbols that will be statically linked (the core
	   symbols) and the number of symbols that will have to be dynamically
	   linked (the overlay symbols). */
	u32 static_symbol_count = 0;
	u32 dynamic_symbol_count = 0;
	for (u32 i = 0; i < table->symbol_count; i++)
		if (table->symbols[i].used)
		{
			if (!table->symbols[i].overlay)
				static_symbol_count++;
			else
				dynamic_symbol_count++;
		}
	
	static const char* section_names[] = {
		"",
		".racdoor.dummy",
		".racdoor.addrtbl",
		".racdoor.relocs",
		".racdoor.symbolmap",
		".racdoor.serial",
		".shstrtab",
		".symtab",
		".strtab"
	};
	
	/* Determine the layout of the object file that is to be written out. */
	u32 file_header_size = sizeof(ElfFileHeader);
	u32 addrtbl_header_size = align32(4 + table->level_count, 4);
	u32 addrtbl_data_size = dynamic_symbol_count * table->overlay_count * 4;
	u32 relocs_size = relocation_count * sizeof(RacdoorRelocation);
	u32 symbolmap_head_size = sizeof(RacdoorSymbolMapHead) + dynamic_symbol_count * sizeof(RacdoorSymbolMapEntry);
	u32 symbolmap_data_size = 0;
	for (u32 i = 0; i < table->symbol_count; i++)
		if (table->symbols[i].used && table->symbols[i].overlay)
			symbolmap_data_size += strlen(table->symbols[i].name) + 1;
	symbolmap_data_size = align32(symbolmap_data_size, 4);
	u32 serial_size = strlen(serial) + 1;
	u32 shstrtab_size = 0;
	for (u32 i = 0; i < ARRAY_SIZE(section_names); i++)
		shstrtab_size += strlen(section_names[i]) + 1;
	shstrtab_size = align32(shstrtab_size, 4);
	u32 section_headers_size = ARRAY_SIZE(section_names) * sizeof(ElfSectionHeader);
	u32 symtab_size = (1 + static_symbol_count) * sizeof(ElfSymbol);
	u32 strtab_size = 1;
	for (u32 i = 0; i < table->symbol_count; i++)
		if (table->symbols[i].used && !table->symbols[i].overlay)
			strtab_size += strlen(table->symbols[i].name) + 1;
	
	u32 file_header_offset = 0;
	u32 addrtbl_header_offset = file_header_offset + file_header_size;
	u32 addrtbl_data_offset = addrtbl_header_offset + addrtbl_header_size;
	u32 relocs_offset = addrtbl_data_offset + addrtbl_data_size;
	u32 symbolmap_head_offset = relocs_offset + relocs_size;
	u32 symbolmap_data_offset = symbolmap_head_offset + symbolmap_head_size;
	u32 serial_offset = symbolmap_data_offset + symbolmap_data_size;
	u32 shstrtab_offset = serial_offset + serial_size;
	u32 section_headers_offset = shstrtab_offset + shstrtab_size;
	u32 symtab_offset = section_headers_offset + section_headers_size;
	u32 strtab_offset = symtab_offset + symtab_size;
	
	u32 file_size = strtab_offset + strtab_size;
	
	Buffer buffer;
	buffer.data = checked_malloc(file_size);
	buffer.size = file_size;
	
	memset(buffer.data, 0, file_size);
	
	/* Fill in the file header. */
	ElfFileHeader* header = (ElfFileHeader*) &buffer.data[file_header_offset];
	header->ident_magic = 0x464c457f;
	header->ident_class = 0x01;
	header->ident_endianess = 0x01;
	header->ident_version = 0x01;
	header->type = 1; /* REL */
	header->machine = 0x08; /* MIPS */
	header->version = 0x01;
	header->shoff = section_headers_offset;
	header->ehsize = sizeof(ElfFileHeader);
	header->shentsize = sizeof(ElfSectionHeader);
	header->shnum = ARRAY_SIZE(section_names);
	header->shstrndx = find_string(".shstrtab", section_names, ARRAY_SIZE(section_names));
	
	/* Fill in the runtime linking table. */
	u8* addrtbl_head = (u8*)  &buffer.data[addrtbl_header_offset] ;
	*(u32*) addrtbl_head = addrtbl_header_size | (dynamic_symbol_count << 8);
	for (u32 i = 4; i < addrtbl_header_size; i++)
		addrtbl_head[i] = (u8) table->levels[i - 4];
	
	u32* addrtbl_data = (u32*) &buffer.data[addrtbl_data_offset];
	for (u32 i = 0; i < table->overlay_count; i++)
	{
		u32 overlay_base = i * dynamic_symbol_count;
		u32 offset = 0;
		for (u32 j = 0; j < table->symbol_count; j++)
			if (table->symbols[j].used && table->symbols[j].overlay)
				addrtbl_data[overlay_base + offset++] = table->symbols[j].overlay_addresses[i];
	}
	
	/* Fill in the symbol mapping table. */
	RacdoorSymbolMapHead* symbolmap = (RacdoorSymbolMapHead*) &buffer.data[symbolmap_head_offset];
	symbolmap->symbol_count = dynamic_symbol_count;
	
	u32 string_offset = 0;
	for (u32 i = 0; i < table->symbol_count; i++)
	{
		if (table->symbols[i].used && table->symbols[i].overlay)
		{
			symbolmap->entries[table->symbols[i].runtime_index].string_offset = symbolmap_head_size + string_offset;
			symbolmap->entries[table->symbols[i].runtime_index].runtime_index = table->symbols[i].runtime_index;
			
			strcpy(&buffer.data[symbolmap_head_offset + symbolmap_head_size + string_offset], table->symbols[i].name);
			
			string_offset += strlen(table->symbols[i].name) + 1;
		}
	}
	
	/* Sort the symbol mapping table using strcmp. */
	string_section = &buffer.data[symbolmap_head_offset];
	qsort(symbolmap->entries, symbolmap->symbol_count, sizeof(RacdoorSymbolMapEntry), symbolmap_comparator);
	
	/* Fill in the serial number. */
	strcpy(&buffer.data[serial_offset], serial);
	
	/* Fill in the section header names. */
	char* shstrtab = &buffer.data[shstrtab_offset];
	for (u32 i = 0; i < ARRAY_SIZE(section_names); i++)
	{
		strcpy(shstrtab, section_names[i]);
		shstrtab += strlen(section_names[i]);
		*shstrtab++ = 0;
	}
	
	ElfSectionHeader sections[] = {
		{},
		/* .racdoor.dummy */ {
			.type = SHT_PROGBITS,
			.offset = 0,
			.size = 0,
			.addralign = 1
		},
		/* .racdoor.addrtbl */ {
			.type = SHT_PROGBITS,
			.offset = addrtbl_header_offset,
			.size = addrtbl_header_size + addrtbl_data_size,
			.addralign = 4
		},
		/* .racdoor.relocs */ {
			.type = SHT_PROGBITS,
			.offset = relocs_offset,
			.size = relocs_size,
			.addralign = 4,
			.entsize = sizeof(RacdoorRelocation)
		},
		/* .racdoor.symbolmap */ {
			.type = SHT_PROGBITS,
			.offset = symbolmap_head_offset,
			.size = symbolmap_head_size + symbolmap_data_size,
			.addralign = 4
		},
		/* .racdoor.serial */ {
			.type = SHT_PROGBITS,
			.offset = serial_offset,
			.size = serial_size,
			.addralign = 1
		},
		/* .shstrtab */ {
			.type = SHT_STRTAB,
			.offset = shstrtab_offset,
			.size = shstrtab_size,
			.addralign = 1
		},
		/* .symtab */ {
			.type = SHT_SYMTAB,
			.offset = symtab_offset,
			.size = symtab_size,
			.link = find_string(".strtab", section_names, ARRAY_SIZE(section_names)),
			.addralign = 4,
			.entsize = sizeof(ElfSymbol)
		},
		/* .strtab */ {
			.type = SHT_STRTAB,
			.offset = strtab_offset,
			.size = strtab_size,
			.addralign = 1
		}
	};
	
	/* Fill in the section headers. */
	ElfSectionHeader* section_headers = (ElfSectionHeader*) &buffer.data[section_headers_offset];
	for (u32 i = 1; i < ARRAY_SIZE(section_names); i++)
	{
		section_headers[i].name = section_headers[i - 1].name + strlen(section_names[i - 1]) + 1;
		section_headers[i].type = sections[i].type;
		section_headers[i].offset = sections[i].offset;
		section_headers[i].size = sections[i].size;
		section_headers[i].link = sections[i].link;
		section_headers[i].addralign = sections[i].addralign;
		section_headers[i].entsize = sections[i].entsize;
	}
	
	/* Fill in the symbol table for static linking. */
	ElfSymbol* symtab = (ElfSymbol*) &buffer.data[symtab_offset];
	char* strtab = &buffer.data[strtab_offset];
	
	ElfSymbol* symbol = symtab + 1;
	u32 name_offset = 1;
	
	for (u32 i = 0; i < table->symbol_count; i++)
	{
		if (table->symbols[i].used && !table->symbols[i].overlay)
		{
			symbol->name = name_offset;
			symbol->value = table->symbols[i].core_address;
			symbol->size = table->symbols[i].size;
			symbol->info = table->symbols[i].type | (STB_GLOBAL << 4);
			symbol->shndx = find_string(".racdoor.dummy", section_names, ARRAY_SIZE(section_names));
			
			strcpy(strtab + symbol->name, table->symbols[i].name);
			
			name_offset += strlen(table->symbols[i].name) + 1;
			symbol++;
		}
	}
	
	return buffer;
}

static int symbolmap_comparator(const void* lhs, const void* rhs)
{
	const char* lhs_string = &string_section[((RacdoorSymbolMapEntry*) lhs)->string_offset];
	const char* rhs_string = &string_section[((RacdoorSymbolMapEntry*) rhs)->string_offset];
	
	return strcmp(lhs_string, rhs_string);
}

static u32 find_string(const char* string, const char** array, u32 count)
{
	for (u32 i = 0; i < count; i++)
		if (strcmp(string, array[i]) == 0)
			return i;
	
	ERROR("No '%s' string exists in the array.\n", string);
}

#include <racdoor/buffer.h>
#include <racdoor/elf.h>
#include <racdoor/util.h>

#include <stdint.h>
#include <string.h>

#define MAX_OVERLAYS 100

typedef struct {
	const char* name;
	const char* comment;
	ElfSymbolType type;
	uint32_t size;
	uint32_t core_address;
	uint32_t overlay_addresses[MAX_OVERLAYS];
	int32_t runtime_index;
	int8_t overlay;
	int8_t used;
} Symbol;

#define MAX_LEVELS 100

typedef struct {
	int32_t overlay_count;
	uint8_t levels[MAX_LEVELS];
	int32_t level_count;
	Symbol* symbols;
	int32_t symbol_count;
} SymbolTable;

static SymbolTable parse_table(Buffer input);
static int32_t parse_object_file(SymbolTable* table, Buffer object);
static void map_symbols_to_runtime_indices(SymbolTable* table);
static void print_table(SymbolTable* table);
static Buffer build_object_file(SymbolTable* table, int32_t relocation_count);
static int symbolmap_comparator(const void* lhs, const void* rhs);

static char* string_section; /* Used by symbolmap_comparator. */

int main(int argc, char** argv)
{
	Buffer* input_objects = checked_malloc(argc * sizeof(Buffer));
	int32_t input_object_count = 0;
	Buffer input_table = {};
	const char* output_object_path = NULL;
	int verbose = 0;
	
	/* Parse the command line arguments. */
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-t") == 0)
			input_table = read_file(argv[++i]);
		else if (strcmp(argv[i], "-o") == 0)
			output_object_path = argv[++i];
		else if (strcmp(argv[i], "-v") == 0)
			verbose = 1;
		else
			input_objects[input_object_count++] = read_file(argv[i]);
	}
	
	CHECK(input_object_count > 0 && input_table.data != NULL && output_object_path != NULL,
		"usage: %s <input objects...> -t <input table> -o <output object> [-v]\n", (argc > 0) ? argv[0] : "tblgen");
	
	/* Parse the input CSV file into a symbol table in memory. */
	SymbolTable table = parse_table(input_table);
	
	/* Check which symbols are actually referenced by the object files being
	   built so we can omit the rest of them from the build later. */
	int32_t relocation_count = 0;
	for (int32_t i = 0; i < input_object_count; i++)
		relocation_count += parse_object_file(&table, input_objects[i]);
	
	/* Determine where each symbol lives in the runtime address table and count
	   the number of symbols to be included. */
	map_symbols_to_runtime_indices(&table);
	
	/* Print out the contents of the symbol table. */
	if (verbose)
		print_table(&table);
	
	/* Build an object file containing the symbols from the CSV file. */
	Buffer elf = build_object_file(&table, relocation_count);
	
	/* Write out the new object file. */
	write_file(output_object_path, elf);
	
	return 0;
}

typedef enum {
	COLUMN_NAME,
	COLUMN_COMMENT,
	COLUMN_TYPE,
	COLUMN_SIZE,
	COLUMN_CORE,
	COLUMN_FIRST_OVERLAY,
	MAX_COLUMNS = 100
} Columns;

static SymbolTable parse_table(Buffer input)
{
	SymbolTable table = {};
	memset(table.levels, 0xff, sizeof(table.levels));
	
	const char* ptr = input.data;
	int32_t column = 0, column_count = 0;
	int seen_name_column = 0;
	
	Columns columns[MAX_COLUMNS];
	
	/* First we iterate over all of the headings and record information about
	   what is stored in each of the columns, and how each of the levels relate
	   to the overlays. */
	while (ptr < input.data + input.size)
	{
		if (ptr == input.data || *(ptr - 1) == ',')
		{
			CHECK(column < MAX_COLUMNS, "Too many columns.\n");
			
			if (strncmp(ptr, "NAME", 4) == 0)
			{
				columns[column] = COLUMN_NAME;
				seen_name_column = 1;
			}
			else if (strncmp(ptr, "COMMENT", 7) == 0)
				columns[column] = COLUMN_COMMENT;
			else if (strncmp(ptr, "TYPE", 4) == 0)
				columns[column] = COLUMN_TYPE;
			else if (strncmp(ptr, "SIZE", 4) == 0)
				columns[column] = COLUMN_SIZE;
			else if (strncmp(ptr, "CORE", 4) == 0)
				columns[column] = COLUMN_CORE;
			else
			{
				columns[column] = COLUMN_FIRST_OVERLAY + table.overlay_count;
				
				/* TODO: Support multiple levels that use the same overlay. */
				char* end = NULL;
				int32_t level = strtoul(ptr, &end, 10);
				CHECK(end != ptr && level > -1 && level < MAX_LEVELS, "Invalid column heading.\n");
				
				table.levels[level] = (uint8_t) table.overlay_count;
				table.level_count = MAX(level + 1, table.level_count);
				table.overlay_count++;
			}
			
			column++;
		}
		
		if (*ptr == '\n')
			break;
		
		ptr++;
	}
	
	column_count = column;
	
	CHECK(*ptr++ == '\n', "Unexpected end of input table file.\n");
	CHECK(seen_name_column != -1, "No NAME column.\n");
	
	/* Each remaining line in the table file will be a symbol. */
	const char* backup_ptr = ptr;
	while (ptr < input.data + input.size)
		if (*ptr++ == '\n')
			table.symbol_count++;
	ptr = backup_ptr;
	
	table.symbols = checked_malloc(table.symbol_count * sizeof(Symbol));
	memset(table.symbols, 0, table.symbol_count * sizeof(Symbol));
	
	int32_t symbol = 0;
	
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
					case COLUMN_SIZE:
					case COLUMN_CORE:
					default:
					{
						if (*ptr == ',')
							break;
						
						char* end = NULL;
						uint32_t value = (uint32_t) strtoul(ptr, &end, 16);
						CHECK(end != ptr, "Invalid %s on line %d.\n",
							columns[column] == COLUMN_SIZE ? "SIZE" : "address",
							symbol + 1);
						
						if (columns[column] == COLUMN_SIZE)
							table.symbols[symbol].size = value;
						if (columns[column] == COLUMN_CORE)
							table.symbols[symbol].core_address = value;
						else
						{
							int32_t overlay = columns[column] - COLUMN_FIRST_OVERLAY;
							CHECK(overlay > -1 && overlay < table.overlay_count, "Invalid column on line %d.\n", symbol + 1);
							
							table.symbols[symbol].overlay_addresses[overlay] = value;
							table.symbols[symbol].overlay = 1;
							
							/* Make sure only a single address is supplied. */
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

static int32_t parse_object_file(SymbolTable* table, Buffer object)
{
	ElfFileHeader* header = buffer_get(object, 0, sizeof(ElfFileHeader), "ELF header");
	int32_t shstrtab_offset = header->shoff + header->shstrndx * sizeof(ElfSectionHeader);
	ElfSectionHeader* shstrtab = buffer_get(object, shstrtab_offset, sizeof(ElfSectionHeader), "section header");
	
	/* Find the symbol table section. */
	ElfSectionHeader* symtab = NULL;
	for (uint32_t i = 0; i < header->shnum; i++)
	{
		int32_t section_offset = header->shoff + i * sizeof(ElfSectionHeader);
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
		int32_t link_offset = header->shoff + symtab->link * sizeof(ElfSectionHeader);
		strtab = buffer_get(object, link_offset, sizeof(ElfSectionHeader), "linked section header");
	}
	
	CHECK(strtab, "No linked string table section.\n");
	
	/* Mark symbols that are referenced in the input object file as used. */
	ElfSymbol* symbols = buffer_get(object, symtab->offset, symtab->size, "symbol table");
	for (int32_t i = 0; i < symtab->size / sizeof(ElfSymbol); i++)
	{
		const char* name = buffer_string(object, strtab->offset + symbols[i].name, "symbol name");
		if (strlen(name) == 0)
			continue;
		
		for (int32_t j = 0; j < table->symbol_count; j++)
			if (table->symbols[j].name && strcmp(table->symbols[j].name, name) == 0)
				table->symbols[j].used = 1;
	}
	
	int32_t relocation_count = 0;
	
	/* Count the number of relocations that will need to be available at runtime
	   so that we can set create a placeholder section for them. */
	for (uint32_t i = 0; i < header->shnum; i++)
	{
		int32_t section_offset = header->shoff + i * sizeof(ElfSectionHeader);
		ElfSectionHeader* section = buffer_get(object, section_offset, sizeof(ElfSectionHeader), "section header");
		if (section->type != SHT_REL)
			continue;
		
		ElfRelocation* relocs = buffer_get(object, section->offset, section->size, "relocations");
		for (int32_t j = 0; j < section->size / sizeof(ElfRelocation); j++)
		{
			unsigned int symbol_index = relocs[j].info >> 8;
			CHECK(symbol_index < symtab->size / sizeof(ElfSymbol), "Invalid symbol index %u.\n", symbol_index);
			ElfSymbol* symbol = &symbols[symbol_index];
			
			const char* name = buffer_string(object, strtab->offset + symbol->name, "symbol name");
			if (strlen(name) == 0)
				continue;
			
			for (int32_t k = 0; k < table->symbol_count; k++)
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
	int32_t next_index = 0;
	for (int32_t i = 0; i < table->symbol_count; i++)
	{
		if (table->symbols[i].used)
		{
			if (table->symbols[i].overlay)
				table->symbols[i].runtime_index = next_index++;
			else
				table->symbols[i].runtime_index = -1;
		}
	}
}

static void print_table(SymbolTable* table)
{
	printf("%d overlays\n", table->overlay_count);
	printf("\n");
	printf("%d levels:\n", table->level_count);
	for (int32_t i = 0; i < table->level_count; i++)
		printf("  [%d] = %d\n", i, table->levels[i]);
	printf("\n");
	printf("%d symbols:\n", table->symbol_count);
	for (int32_t i = 0; i < table->symbol_count; i++)
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
		for (int32_t j = 0; j < table->overlay_count; j++)
			if (table->symbols[i].overlay_addresses[j])
				last = printf("%s overlay[%d] = %x", last ? "," : "", j, table->symbols[i].overlay_addresses[j]);
		printf(" }\n");
	}
}

static Buffer build_object_file(SymbolTable* table, int32_t relocation_count)
{
	/* Count the number of symbols that will be statically linked (the core
	   symbols) and the number of symbols that will have to be dynamically
	   linked (the overlay symbols). */
	int32_t static_symbol_count = 0;
	int32_t dynamic_symbol_count = 0;
	for (int32_t i = 0; i < table->symbol_count; i++)
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
		".shstrtab",
		".symtab",
		".strtab"
	};
	
	/* Determine the layout of the object file that is to be written out. */
	int32_t file_header_size = sizeof(ElfFileHeader);
	int32_t addrtbl_header_size = align32(4 + table->level_count, 4);
	int32_t addrtbl_data_size = dynamic_symbol_count * table->overlay_count * 4;
	int32_t relocs_size = relocation_count * sizeof(RacdoorRelocation);
	int32_t symbolmap_head_size = sizeof(RacdoorSymbolMapHead) + dynamic_symbol_count * sizeof(RacdoorSymbolMapEntry);
	int32_t symbolmap_data_size = 0;
	for (int32_t i = 0; i < table->symbol_count; i++)
		if (table->symbols[i].used && table->symbols[i].overlay)
			symbolmap_data_size += strlen(table->symbols[i].name) + 1;
	symbolmap_data_size = align32(symbolmap_data_size, 4);
	int32_t shstrtab_size = 0;
	for (int32_t i = 0; i < ARRAY_SIZE(section_names); i++)
		shstrtab_size += strlen(section_names[i]) + 1;
	shstrtab_size = align32(shstrtab_size, 4);
	int32_t section_headers_size = ARRAY_SIZE(section_names) * sizeof(ElfSectionHeader);
	int32_t symtab_size = (1 + static_symbol_count) * sizeof(ElfSymbol);
	int32_t strtab_size = 1;
	for (int32_t i = 0; i < table->symbol_count; i++)
		if (table->symbols[i].used && !table->symbols[i].overlay)
			strtab_size += strlen(table->symbols[i].name) + 1;
	
	int32_t file_header_offset = 0;
	int32_t addrtbl_header_offset = file_header_offset + file_header_size;
	int32_t addrtbl_data_offset = addrtbl_header_offset + addrtbl_header_size;
	int32_t relocs_offset = addrtbl_data_offset + addrtbl_data_size;
	int32_t symbolmap_head_offset = relocs_offset + relocs_size;
	int32_t symbolmap_data_offset = symbolmap_head_offset + symbolmap_head_size;
	int32_t shstrtab_offset = symbolmap_data_offset + symbolmap_data_size;
	int32_t section_headers_offset = shstrtab_offset + shstrtab_size;
	int32_t symtab_offset = section_headers_offset + section_headers_size;
	int32_t strtab_offset = symtab_offset + symtab_size;
	
	int32_t file_size = strtab_offset + strtab_size;
	
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
	header->shstrndx = 5; /* .shstrtab */
	
	/* Fill in the runtime linking table. */
	*(uint32_t*) &buffer.data[addrtbl_header_offset] = addrtbl_header_size | (dynamic_symbol_count << 8);
	uint8_t* addrtbl_levels = (uint8_t*) &buffer.data[addrtbl_header_offset + 4];
	for (int32_t i = 4; i < addrtbl_header_size; i++)
		addrtbl_levels[i] = (uint8_t) table->levels[i - 4];
	
	uint32_t* addrtbl_data = (uint32_t*) &buffer.data[addrtbl_data_offset];
	for (int32_t i = 0; i < table->overlay_count; i++)
	{
		int32_t overlay_base = i * dynamic_symbol_count;
		int32_t offset = 0;
		for (int32_t j = 0; j < table->symbol_count; j++)
			if (table->symbols[j].used && table->symbols[j].overlay)
				addrtbl_data[overlay_base + offset++] = table->symbols[j].overlay_addresses[i];
	}
	
	/* Fill in the symbol mapping table. */
	RacdoorSymbolMapHead* symbolmap = (RacdoorSymbolMapHead*) &buffer.data[symbolmap_head_offset];
	symbolmap->symbol_count = dynamic_symbol_count;
	
	int32_t string_offset = 0;
	for (int32_t i = 0; i < table->symbol_count; i++)
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
	
	/* Fill in the section header names. */
	char* shstrtab = &buffer.data[shstrtab_offset];
	for (int32_t i = 0; i < ARRAY_SIZE(section_names); i++)
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
			.link = 7, /* .strtab */
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
	for (int32_t i = 1; i < ARRAY_SIZE(section_names); i++)
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
	int32_t name_offset = 1;
	
	for (int32_t i = 0; i < table->symbol_count; i++)
	{
		if (table->symbols[i].used && !table->symbols[i].overlay)
		{
			symbol->name = name_offset;
			symbol->value = table->symbols[i].core_address;
			symbol->size = table->symbols[i].size;
			symbol->info = table->symbols[i].type | (STB_GLOBAL << 4);
			symbol->shndx = 1; /* .racdoor.dummy */
			
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

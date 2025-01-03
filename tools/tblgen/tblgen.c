/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <racdoor/buffer.h>
#include <racdoor/csv.h>
#include <racdoor/elf.h>
#include <racdoor/util.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static u32 parse_archive_file(SymbolTable* table, Buffer archive);
static u32 parse_object_file(SymbolTable* table, Buffer object);
static void sort_symbols(SymbolTable* table);
static int symbol_comparator(const void* lhs, const void* rhs);
static void map_symbols_to_runtime_indices(SymbolTable* table);
static void print_table(SymbolTable* table);
static Buffer build_object_file(SymbolTable* table, u32 relocation_count, const char* serial);
static int symbolmap_comparator(const void* lhs, const void* rhs);
static u32 find_string(const char* string, const char** array, u32 count);

static char* string_section; /* Used by symbolmap_comparator. */

int main(int argc, char** argv)
{
	Buffer* input_files = checked_malloc(argc * sizeof(Buffer));
	const char** input_file_paths = checked_malloc(argc * sizeof(const char*));
	u32 input_file_count = 0;
	Buffer input_table = {};
	const char* serial = NULL;
	const char* output_object_path = NULL;
	b8 verbose = FALSE;
	
	/* Parse the command line arguments. */
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-t") == 0)
		{
			i++;
			CHECK(i < argc, "Expected input table path.");
			input_table = read_file(argv[i]);
		}
		else if (strcmp(argv[i], "-s") == 0)
		{
			i++;
			CHECK(i < argc, "Expected serial.");
			serial = argv[i];
		}
		else if (strcmp(argv[i], "-o") == 0)
		{
			i++;
			CHECK(i < argc, "Expected output path.");
			output_object_path = argv[i];
		}
		else if (strcmp(argv[i], "-v") == 0)
			verbose = TRUE;
		else
		{
			input_files[input_file_count] = read_file(argv[i]);
			input_file_paths[input_file_count] = argv[i];
			input_file_count++;
		}
	}
	
	CHECK(input_file_count && input_table.data && serial && output_object_path,
		"usage: %s <input archives and objects...> -t <input table> -s <serial> -o <output object> [-v]",
		(argc > 0) ? argv[0] : "tblgen");
	
	/* Parse the input CSV file into a symbol table in memory. */
	SymbolTable table = parse_table(input_table);
	
	/* Check which symbols are actually referenced by the object files being
	   built so we can omit the rest of them from the build later. */
	u32 relocation_count = 0;
	for (u32 i = 0; i < input_file_count; i++)
		if (strncmp(input_files[i].data, "!<arch>\n", 8) == 0)
			relocation_count += parse_archive_file(&table, input_files[i]);
		else if (strncmp(input_files[i].data, "\x7f" "ELF", 4) == 0)
			relocation_count += parse_object_file(&table, input_files[i]);
		else
			ERROR("Cannot determine type of input file '%s'.", input_file_paths[i]);
	
	/* Sort all the symbols by their highest addresses. */
	sort_symbols(&table);
	
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

typedef struct {
	char identifier[16];
	char modified[12];
	char owner[6];
	char group[6];
	char mode[8];
	char file_size[10];
	char end[2];
} ArchiveHeader;

static u32 parse_archive_file(SymbolTable* table, Buffer archive)
{
	u32 offset = 0;
	u32 relocation_count = 0;
	
	CHECK(strncmp(archive.data, "!<arch>\n", 8) == 0, "Invalid archive header.");
	offset += 8;
	
	while (offset < archive.size)
	{
		ArchiveHeader* header = buffer_get(archive, offset, sizeof(ArchiveHeader), "archive header");
		offset += sizeof(ArchiveHeader);
		
		u32 file_size = atoi(header->file_size);
		
		char* identifier_end = (char*) memchr(header->identifier, '/', sizeof(header->identifier));
		CHECK(identifier_end, "Corrupted archive (invalid identifier).");
		
		char identifier[16];
		memcpy(identifier, header->identifier, identifier_end - header->identifier);
		identifier[identifier_end - header->identifier] = '\0';
		
		/* The GNU convention for storing long filenames is to make the
		   identifier field empty except for a decimal index that points into a
		   list of the real filenames. */
		int long_filename = identifier_end == header->identifier
			&& header->identifier[1] >= '0'
			&& header->identifier[1] <= '9';
		
		/* Skip over the global symbol table. */
		if (strcmp(identifier, "") != 0 || long_filename)
			relocation_count += parse_object_file(table, sub_buffer(archive, offset, file_size, "archive data"));
		
		offset += ALIGN(file_size, 2);
	}
	
	return relocation_count;
}

static u32 parse_object_file(SymbolTable* table, Buffer object)
{
	ElfFileHeader* header = buffer_get(object, 0, sizeof(ElfFileHeader), "ELF header");
	ElfSectionHeader* symtab = lookup_section(object, ".symtab");
	
	/* Find the string table section. */
	ElfSectionHeader* strtab = NULL;
	if (symtab->link != 0)
	{
		u32 link_offset = header->shoff + symtab->link * sizeof(ElfSectionHeader);
		strtab = buffer_get(object, link_offset, sizeof(ElfSectionHeader), "linked section header");
	}
	
	CHECK(strtab, "No linked string table section.");
	
	/* Mark symbols that are referenced in the input object file as used. */
	ElfSymbol* symbols = buffer_get(object, symtab->offset, symtab->size, "symbol table");
	for (u32 i = 0; i < symtab->size / sizeof(ElfSymbol); i++)
	{
		const char* name = buffer_string(object, strtab->offset + symbols[i].name, "symbol name");
		if (strlen(name) == 0)
			continue;
		
		for (u32 j = 0; j < table->symbol_count; j++)
			if (table->symbols[j].name && strcmp(table->symbols[j].name, name) == 0)
				table->symbols[j].used = TRUE;
	}
	
	u32 relocation_count = 0;
	
	/* Count the number of relocations that will need to be available at runtime
	   so that we can create a placeholder section for them. */
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
			CHECK(symbol_index < symtab->size / sizeof(ElfSymbol), "Invalid symbol index %u.", symbol_index);
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

static void sort_symbols(SymbolTable* table)
{
	for (u32 i = 0; i < table->symbol_count; i++)
	{
		Symbol* symbol = &table->symbols[i];
		symbol->temp_address = MAX(symbol->core_address, symbol->temp_address);
		symbol->temp_address = MAX(symbol->spcore_address, symbol->temp_address);
		symbol->temp_address = MAX(symbol->mpcore_address, symbol->temp_address);
		symbol->temp_address = MAX(symbol->frontend_address, symbol->temp_address);
		symbol->temp_address = MAX(symbol->frontbin_address, symbol->temp_address);
		for (u32 j = 0; j < table->overlay_count; j++)
			symbol->temp_address = MAX(symbol->overlay_addresses[j], symbol->temp_address);
	}
	
	qsort(table->symbols, table->symbol_count, sizeof(Symbol), symbol_comparator);
}

static int symbol_comparator(const void* lhs, const void* rhs)
{
	return ((Symbol*) lhs)->temp_address > ((Symbol*) rhs)->temp_address;
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
		".racdoor.overlaymap",
		".racdoor.addrtbl",
		".racdoor.fastdecompress",
		".racdoor.relocs",
		".racdoor.symbolmap",
		".racdoor.serial",
		".shstrtab",
		".symtab",
		".strtab"
	};
	
	/* Determine the layout of the object file that is to be written out. */
	u32 file_header_size = sizeof(ElfFileHeader);
	u32 overlaymap_size = table->level_count;
	u32 addrtbl_size = 4 + dynamic_symbol_count * table->overlay_count * 4;
	u32 fastdecompress_size = table->level_count * 4;
	u32 relocs_size = (relocation_count + 1) * sizeof(RacdoorRelocation);
	u32 symbolmap_head_size = sizeof(RacdoorSymbolMapHead) + dynamic_symbol_count * sizeof(RacdoorSymbolMapEntry);
	u32 symbolmap_data_size = 0;
	for (u32 i = 0; i < table->symbol_count; i++)
		if (table->symbols[i].used && table->symbols[i].overlay)
			symbolmap_data_size += strlen(table->symbols[i].name) + 1;
	u32 serial_size = strlen(serial) + 1;
	u32 shstrtab_size = 0;
	for (u32 i = 0; i < ARRAY_SIZE(section_names); i++)
		shstrtab_size += strlen(section_names[i]) + 1;
	u32 section_headers_size = ARRAY_SIZE(section_names) * sizeof(ElfSectionHeader);
	u32 symtab_size = (1 + static_symbol_count) * sizeof(ElfSymbol);
	u32 strtab_size = 1;
	for (u32 i = 0; i < table->symbol_count; i++)
		if (table->symbols[i].used && !table->symbols[i].overlay)
			strtab_size += strlen(table->symbols[i].name) + 1;
	
	u32 file_header_offset = 0;
	u32 overlaymap_offset = ALIGN(file_header_offset + file_header_size, 4);
	u32 addrtbl_offset = ALIGN(overlaymap_offset + overlaymap_size, 4);
	u32 fastdecompress_offset = ALIGN(addrtbl_offset + addrtbl_size, 4);
	u32 relocs_offset = ALIGN(fastdecompress_offset + fastdecompress_size, 4);
	u32 symbolmap_head_offset = ALIGN(relocs_offset + relocs_size, 4);
	u32 symbolmap_data_offset = ALIGN(symbolmap_head_offset + symbolmap_head_size, 4);
	u32 serial_offset = ALIGN(symbolmap_data_offset + symbolmap_data_size, 4);
	u32 shstrtab_offset = ALIGN(serial_offset + serial_size, 4);
	u32 section_headers_offset = ALIGN(shstrtab_offset + shstrtab_size, 4);
	u32 symtab_offset = ALIGN(section_headers_offset + section_headers_size, 4);
	u32 strtab_offset = ALIGN(symtab_offset + symtab_size, 4);
	
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
	
	/* Fill in the level to overlay mapping table. */
	u8* overlaymap = (u8*) &buffer.data[overlaymap_offset];
	for (u32 i = 0; i < table->level_count; i++)
		overlaymap[i] = table->levels[i];
	
	/* Fill in the runtime linking table. */
	u32* addrtbl = (u32*) &buffer.data[addrtbl_offset];
	*addrtbl = dynamic_symbol_count;
	for (u32 i = 0; i < table->overlay_count; i++)
	{
		u32 overlay_base = i * dynamic_symbol_count;
		u32 offset = 0;
		for (u32 j = 0; j < table->symbol_count; j++)
			if (table->symbols[j].used && table->symbols[j].overlay)
				addrtbl[1 + overlay_base + offset++] = table->symbols[j].overlay_addresses[i];
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
	
	/* Fill in the array of FastDecompress function pointers. */
	Symbol* fastdecompress = NULL;
	for (u32 i = 0; i < table->symbol_count; i++)
	{
		if (strcmp(table->symbols[i].name, "FastDecompress") == 0)
		{
			fastdecompress = &table->symbols[i];
			break;
		}
	}
	
	CHECK(fastdecompress, "No 'FastDecompress' symbol found in CSV table.");
	
	u32* fastdecompress_funcs = (u32*) &buffer.data[fastdecompress_offset];
	for (u32 i = 0; i < table->overlay_count; i++)
		fastdecompress_funcs[i] = fastdecompress->overlay_addresses[i];
	
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
		/* .racdoor.overlaymap */ {
			.type = SHT_PROGBITS,
			.offset = overlaymap_offset,
			.size = overlaymap_size,
			.addralign = 1
		},
		/* .racdoor.addrtbl */ {
			.type = SHT_PROGBITS,
			.offset = addrtbl_offset,
			.size = addrtbl_size,
			.addralign = 4
		},
		/* .racdoor.fastdecompress */ {
			.type = SHT_PROGBITS,
			.offset = fastdecompress_offset,
			.size = fastdecompress_size,
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
	
	ERROR("No '%s' string exists in the array.", string);
}

/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <racdoor/buffer.h>
#include <racdoor/csv.h>
#include <racdoor/elf.h>
#include <racdoor/util.h>

#include <ctype.h>
#include <stdint.h>
#include <string.h>

static void create_section_symbols(SymbolTable* table, Buffer elf);
static void fill_in_section_symbols(SymbolTable* table, Buffer elf);
static const char* format_symbol_name(const char* section_name, b8 append_end_suffix);
static void print_row(Symbol* symbol, Column columns[MAX_COLUMNS], u32 column_count);

int main(int argc, char** argv)
{
	const char* csv_header = NULL;
	Buffer boot_elf = {};
	Buffer multiplayer_elf = {};
	Buffer frontbin_elf = {};
	Buffer* overlays = checked_malloc(argc * sizeof(Buffer));
	u32* overlay_level_numbers = checked_malloc(argc * sizeof(u32));
	u32 overlay_count = 0;
	
	/* Parse the command line arguments and load the input files. */
	for (u32 i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0)
		{
			i++;
			CHECK(i < argc, "Expected CSV header.");
			csv_header = argv[i];
		}
		else if (strcmp(argv[i], "-b") == 0)
		{
			i++;
			CHECK(i < argc, "Expected boot ELF path.");
			boot_elf = read_file(argv[i]);
		}
		else if (strcmp(argv[i], "-m") == 0)
		{
			i++;
			CHECK(i < argc, "Expected multiplayer ELF path.");
			multiplayer_elf = read_file(argv[i]);
		}
		else if (strcmp(argv[i], "-f") == 0)
		{
			i++;
			CHECK(i < argc, "Expected frontbin ELF path.");
			frontbin_elf = read_file(argv[i]);
		}
		else
		{
			char directory_path[1024];
			strncpy(directory_path, argv[i], sizeof(directory_path));
			directory_path[sizeof(directory_path) - 1] = '\0';
			
			if (directory_path[strlen(directory_path) - 1] == '/')
				directory_path[strlen(directory_path) - 1] = '\0';
			
			const char* path_separator = strrchr(directory_path, '/');
			
			const char* directory_name;
			if (directory_name)
				directory_name = path_separator + 1;
			else
				directory_name = directory_path;
			
			/* Ignore .asset files that may be included due to a shell glob. */
			if (strstr(directory_name, ".asset") != NULL)
				continue;
			
			CHECK(directory_name[0] >= '0' && directory_name[0] <= '9', "Level directory name must start with the level number.");
			
			char overlay_path[1024];
			snprintf(overlay_path, sizeof(overlay_path), "%s/overlay.elf", argv[i]);
			
			overlays[overlay_count] = read_file(overlay_path);
			overlay_level_numbers[overlay_count] = atoi(directory_name);
			
			overlay_count++;
		}
	}
	
	CHECK(csv_header && boot_elf.data,
		"usage: %s -h <csv header> -b <boot elf path> [-m <multiplayer elf>] [-f <frontbin elf path>] "
		"<level directories...>",
		(argc > 0) ? argv[0] : "sectiondump");
	
	ElfSectionHeader* boot_elf_core_text = lookup_section(boot_elf, "core.text");
	CHECK(boot_elf_core_text, "No core.text section in boot ELF, it is either packed or not a R&C boot ELF.");
	
	ElfSectionHeader* boot_elf_lit = lookup_section(boot_elf, ".lit");
	CHECK(boot_elf_lit, "No .lit section in boot ELF, it is either packed or not a R&C boot ELF.");
	
	u32 sp_overlay_start_address = boot_elf_lit->addr;
	u32 mp_overlay_start_address = UINT32_MAX;
	
	SymbolTable table;
	Column columns[MAX_COLUMNS];
	
	u32 column_count = parse_table_header(&csv_header, &table, columns, FALSE);
	
	/* Allocate memory for the symbols. */
	create_section_symbols(&table, boot_elf);
	
	/* Fill in all the addresses. The fill_in_section_symbols function stores
	   its results in the temp_address field initially, then it's copied to the
	   correct field afterwards. */
	fill_in_section_symbols(&table, boot_elf);
	for (u32 i = 0; i < table.symbol_count; i++)
	{
		Symbol* symbol = &table.symbols[i];
		if (!symbol->used)
			continue;
		
		if (symbol->temp_address >= sp_overlay_start_address)
			symbol->frontend_address = symbol->temp_address;
		else if (multiplayer_elf.data)
			symbol->spcore_address = symbol->temp_address;
		else
			symbol->core_address = symbol->temp_address;
	}
		
	if (multiplayer_elf.data)
	{
	
		ElfSectionHeader* mp_elf_core_text = lookup_section(multiplayer_elf, "core.text");
		CHECK(mp_elf_core_text, "No core.text section in multiplayer ELF, it is either packed or not a R&C boot ELF.");
	
		ElfSectionHeader* mp_elf_lit = lookup_section(multiplayer_elf, ".lit");
		CHECK(mp_elf_lit, "No .lit section in multiplayer ELF, it is either packed or not a R&C boot ELF.");
		
		mp_overlay_start_address = mp_elf_lit->addr;
		
		fill_in_section_symbols(&table, multiplayer_elf);
		for (u32 i = 0; i < table.symbol_count; i++)
		{
			Symbol* symbol = &table.symbols[i];
			if (!symbol->used)
				continue;
			
			if (symbol->temp_address >= mp_overlay_start_address)
				symbol->mpfrontend_address = symbol->temp_address;
			else
				symbol->mpcore_address = symbol->temp_address;
		}
	}
	
	if(frontbin_elf.data)
	{
		fill_in_section_symbols(&table, frontbin_elf);
		for (u32 i = 0; i < table.symbol_count; i++)
		{
			Symbol* symbol = &table.symbols[i];
			if (!symbol->used)
				continue;
			
			symbol->frontbin_address = symbol->temp_address;
		}
	}
	
	for (u32 i = 0; i < overlay_count; i++)
	{
		CHECK(overlay_level_numbers[i] < table.level_count, "Level number from directory name out of bounds.");
		u32 overlay_index = table.levels[overlay_level_numbers[i]];
		
		fill_in_section_symbols(&table, overlays[i]);
		for (u32 i = 0; i < table.symbol_count; i++)
		{
			Symbol* symbol = &table.symbols[i];
			if (!symbol->used)
				continue;
			
			symbol->overlay_addresses[overlay_index] = symbol->temp_address;
		}
	}
	
	/* Print out the newly generated CSV data. */
	for (u32 i = 0; i < table.symbol_count; i++)
		print_row(&table.symbols[i], columns, column_count);
	
	return 0;
}

static void create_section_symbols(SymbolTable* table, Buffer elf)
{
	ElfFileHeader* header = parse_elf_header(elf);
	u32 shstrtab_offset = header->shoff + header->shstrndx * sizeof(ElfSectionHeader);
	ElfSectionHeader* shstrtab = buffer_get(elf, shstrtab_offset, sizeof(ElfSectionHeader), "shstr section header");
	
	/* Count the number of symbols to be allocated. */
	u32 symbol_count = 0;
	for (u32 i = 0; i < header->shnum; i++)
	{
		u32 section_offset = header->shoff + i * sizeof(ElfSectionHeader);
		ElfSectionHeader* section_header = buffer_get(elf, section_offset, sizeof(ElfSectionHeader), "section header");
		
		if (section_header->addr != 0)
			symbol_count += 2;
	}
	
	table->symbols = malloc(symbol_count * sizeof(Symbol));
	table->symbol_count = symbol_count;
	
	memset(table->symbols, 0, symbol_count * sizeof(Symbol));
	
	/* Fill in the section names. */
	u32 symbol_index = 0;
	for (u32 i = 0; i < header->shnum; i++)
	{
		u32 section_offset = header->shoff + i * sizeof(ElfSectionHeader);
		ElfSectionHeader* section_header = buffer_get(elf, section_offset, sizeof(ElfSectionHeader), "section header");
		
		if (section_header->addr == 0)
			continue;
		
		const char* name = buffer_string(elf, shstrtab->offset + section_header->name, "section name");
		
		Symbol* start_symbol = &table->symbols[symbol_index++];
		start_symbol->name = format_symbol_name(name, FALSE);
		start_symbol->temp_address = section_header->addr;
		
		Symbol* end_symbol = &table->symbols[symbol_index++];
		end_symbol->name = format_symbol_name(name, TRUE);
		end_symbol->temp_address = section_header->addr + section_header->size;
	}
}

static void fill_in_section_symbols(SymbolTable* table, Buffer elf)
{
	for(u32 i = 0; i < table->symbol_count; i++)
		table->symbols[i].used = FALSE;
	
	ElfFileHeader* header = parse_elf_header(elf);
	u32 shstrtab_offset = header->shoff + header->shstrndx * sizeof(ElfSectionHeader);
	ElfSectionHeader* shstrtab = buffer_get(elf, shstrtab_offset, sizeof(ElfSectionHeader), "shstr section header");
	
	for (u32 i = 0; i < header->shnum; i++)
	{
		u32 section_offset = header->shoff + i * sizeof(ElfSectionHeader);
		ElfSectionHeader* section_header = buffer_get(elf, section_offset, sizeof(ElfSectionHeader), "section header");
		
		if (section_header->addr == 0)
			continue;
		
		const char* name = buffer_string(elf, shstrtab->offset + section_header->name, "section name");
		
		const char* start_name = format_symbol_name(name, FALSE);
		Symbol* start_symbol = NULL;
		for (u32 j = 0; j < table->symbol_count; j++)
			if (strcmp(table->symbols[j].name, start_name) == 0)
				start_symbol = &table->symbols[j];
		
		if (start_symbol)
		{
			start_symbol->temp_address = section_header->addr;
			start_symbol->used = TRUE;
		}
		
		const char* end_name = format_symbol_name(name, TRUE);
		Symbol* end_symbol = NULL;
		for (u32 j = 0; j < table->symbol_count; j++)
			if (strcmp(table->symbols[j].name, end_name) == 0)
				end_symbol = &table->symbols[j];
		
		if (end_symbol)
		{
			end_symbol->temp_address = section_header->addr + section_header->size;
			end_symbol->used = TRUE;
		}
	}
}

static const char* format_symbol_name(const char* section_name, b8 append_end_suffix)
{
	const char* prefix = "_section_";
	const char* end_suffix = "_end";
	
	char* symbol_name = malloc(strlen(prefix) + strlen(section_name) + strlen(end_suffix) + 1);
	strcpy(symbol_name, prefix);
	
	u32 string_pos = strlen(prefix);
	for (size_t i = 0; i < strlen(section_name); i++)
		if (isalnum(section_name[i]))
			symbol_name[string_pos++] = section_name[i];
	
	if (append_end_suffix)
		strcpy(symbol_name + string_pos, end_suffix);
	else
		symbol_name[string_pos] = '\0';
	
	return symbol_name;
}

static void print_row(Symbol* symbol, Column columns[MAX_COLUMNS], u32 column_count)
{
	for (u32 i = 0; i < column_count; i++)
	{
		if (i != 0)
			printf(",");
		
		u32 address = 0;
		if (columns[i] >= COLUMN_FIRST_OVERLAY)
		{
			address = symbol->overlay_addresses[columns[i] - COLUMN_FIRST_OVERLAY];
		}
		else
		{
			switch (columns[i])
			{
				case COLUMN_NAME:
					printf("%s", symbol->name);
					break;
				case COLUMN_TYPE:
					printf("NOTYPE");
					break;
				case COLUMN_CORE:
					address = symbol->core_address;
					break;
				case COLUMN_SPCORE:
					address = symbol->spcore_address;
					break;
				case COLUMN_MPCORE:
					address = symbol->mpcore_address;
					break;
				case COLUMN_FRONTEND:
					address = symbol->frontend_address;
					break;
				case COLUMN_FRONTBIN:
					address = symbol->frontbin_address;
					break;
				default:
					break;
			}
		}
		
		if (address)
			printf("%x", address);
	}
	printf("\n");
}

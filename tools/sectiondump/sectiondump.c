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

#define MAX_CLASSES 65536

typedef enum {
	MODE_NONE,
	MODE_SECTION_HEADERS,
	MODE_VTBL,
	MODE_CAMVTBL,
	MODE_SNDVTBL
} Mode;

typedef struct {
	Mode mode;
	const char* csv_header;
	Buffer boot_elf;
	Buffer mp_elf;
	Buffer frontbin_elf;
	Buffer* overlays;
	u32* overlay_level_numbers;
	u32 overlay_count;
} Input;

Input parse_args(int argc, char** argv);

static void dump_sections(SymbolTable* table, Input* input);
static void create_section_symbols(SymbolTable* table, Buffer elf);
static void fill_in_section_symbols(SymbolTable* table, Buffer elf);
static const char* format_symbol_name(const char* section_name, b8 append_end_suffix);

static void dump_vtbl(
	SymbolTable* table,
	Input* input,
	const char* section_name,
	u32 stride,
	const char** function_names,
	u32 function_count);

static void enumerate_classes(b8 classes[MAX_CLASSES], Buffer elf, const char* section_name, u32 stride);

static void fill_in_class_symbols(
	SymbolTable* table,
	Buffer elf,
	const char* section_name,
	u32 stride,
	const char** function_names,
	u32 function_count);

static const char* format_class_symbol_name(const char* function_name, u32 class_number);

static void print_row(Symbol* symbol, Column columns[MAX_COLUMNS], u32 column_count, const char* symbol_type);

int main(int argc, char** argv)
{
	/* Parse the command line arguments and load the input files. */
	Input input = parse_args(argc, argv);
	
	SymbolTable table;
	Column columns[MAX_COLUMNS];
	
	const char* backup_csv_header = input.csv_header;
	u32 column_count = parse_table_header(&input.csv_header, &table, columns, FALSE);
	
	switch (input.mode)
	{
		case MODE_SECTION_HEADERS:
		{
			dump_sections(&table, &input);
			break;
		}
		case MODE_VTBL:
		{
			const char* function_names[] = {"UpdateMoby_%d"};
			dump_vtbl(&table, &input, "lvl.vtbl", 12, function_names, ARRAY_SIZE(function_names));
			break;
		}
		case MODE_CAMVTBL:
		{
			const char* function_names[] = {"InitCamera_%d", "ActivateCamera_%d", "UpdateCamera_%d", "ExitCamera_%d"};
			dump_vtbl(&table, &input, "lvl.camvtbl", 20, function_names, ARRAY_SIZE(function_names));
			break;
		}
		case MODE_SNDVTBL:
		{
			const char* function_names[] = {"UpdateSound_%d"};
			dump_vtbl(&table, &input, "lvl.sndvtbl", 8, function_names, ARRAY_SIZE(function_names));
			break;
		}
		default:
		{
			break;
		}
	}
	
	/* Print out the newly generated CSV data. */
	const char* symbol_type;
	if (input.mode == MODE_SECTION_HEADERS)
		symbol_type = "NOTYPE";
	else
		symbol_type = "FUNC";
	
	printf("%s\n", backup_csv_header);
	for (u32 i = 0; i < table.symbol_count; i++)
		print_row(&table.symbols[i], columns, column_count, symbol_type);
	
	return 0;
}

Input parse_args(int argc, char** argv)
{
	Input input;
	
	memset(&input, 0, sizeof(input));
	
	input.mode = MODE_NONE;
	input.overlays = checked_malloc(argc * sizeof(Buffer));
	input.overlay_level_numbers = checked_malloc(argc * sizeof(u32));
	
	for (u32 i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-e") == 0)
		{
			input.mode = MODE_SECTION_HEADERS;
		}
		else if (strcmp(argv[i], "-v") == 0)
		{
			input.mode = MODE_VTBL;
		}
		else if (strcmp(argv[i], "-c") == 0)
		{
			input.mode = MODE_CAMVTBL;
		}
		else if (strcmp(argv[i], "-s") == 0)
		{
			input.mode = MODE_SNDVTBL;
		}
		else if (strcmp(argv[i], "-h") == 0)
		{
			i++;
			CHECK(i < argc, "Expected CSV header.");
			input.csv_header = argv[i];
		}
		else if (strcmp(argv[i], "-b") == 0)
		{
			i++;
			CHECK(i < argc, "Expected boot ELF path.");
			input.boot_elf = read_file(argv[i]);
		}
		else if (strcmp(argv[i], "-m") == 0)
		{
			i++;
			CHECK(i < argc, "Expected multiplayer ELF path.");
			input.mp_elf = read_file(argv[i]);
		}
		else if (strcmp(argv[i], "-f") == 0)
		{
			i++;
			CHECK(i < argc, "Expected frontbin ELF path.");
			input.frontbin_elf = read_file(argv[i]);
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
			
			CHECK(directory_name[0] >= '0' && directory_name[0] <= '9',
				"Level directory name must start with the level number.");
			
			char overlay_path[1024];
			snprintf(overlay_path, sizeof(overlay_path), "%s/overlay.elf", argv[i]);
			
			input.overlays[input.overlay_count] = read_file(overlay_path);
			input.overlay_level_numbers[input.overlay_count] = atoi(directory_name);
			
			input.overlay_count++;
		}
	}
	
	CHECK(input.mode != MODE_NONE && input.csv_header && input.boot_elf.data,
		"usage: %s -e|-v|-c|-s -h <csv header> -b <boot elf path> [-m <multiplayer elf>] [-f <frontbin elf path>] "
		"<level directories...>\n\n"
		"This tool generates CSV table files from all the ELF files output by wrench for\n"
		"a given build of a R&C game. Use of shell globbing is recommended for passing in\n"
		"the list of level directories. The modes are as follows:\n"
		"\n"
		"-e Dump ELF section headers.\n"
		"-v Dump the moby function (lvl.vtbl) section.\n"
		"-c Dump the camera function (lvl.camvtbl) section.\n"
		"-s Dump the sound function (lvl.sndvtbl) section.",
		(argc > 0) ? argv[0] : "sectiondump");
	
	return input;
}

static void dump_sections(SymbolTable* table, Input* input)
{
	ElfSectionHeader* boot_elf_core_text = lookup_section(input->boot_elf, "core.text");
	CHECK(boot_elf_core_text, "No core.text section in boot ELF, it is either packed or not a R&C boot ELF.");
	
	ElfSectionHeader* boot_elf_lit = lookup_section(input->boot_elf, ".lit");
	CHECK(boot_elf_lit, "No .lit section in boot ELF, it is either packed or not a R&C boot ELF.");
	
	u32 sp_overlay_start_address = boot_elf_lit->addr;
	u32 mp_overlay_start_address = UINT32_MAX;
	
	/* Allocate memory for the symbols. */
	create_section_symbols(table, input->boot_elf);
	
	/* Fill in all the addresses. The fill_in_section_symbols function stores
	   its results in the temp_address field initially, then it's copied to the
	   correct field afterwards. */
	fill_in_section_symbols(table, input->boot_elf);
	for (u32 i = 0; i < table->symbol_count; i++)
	{
		Symbol* symbol = &table->symbols[i];
		if (!symbol->used)
			continue;
		
		if (symbol->temp_address >= sp_overlay_start_address)
			symbol->frontend_address = symbol->temp_address;
		else if (input->mp_elf.data)
			symbol->spcore_address = symbol->temp_address;
		else
			symbol->core_address = symbol->temp_address;
	}
		
	if (input->mp_elf.data)
	{
		ElfSectionHeader* mp_elf_core_text = lookup_section(input->mp_elf, "core.text");
		CHECK(mp_elf_core_text, "No core.text section in multiplayer ELF, it is either packed or not a R&C boot ELF.");
	
		ElfSectionHeader* mp_elf_lit = lookup_section(input->mp_elf, ".lit");
		CHECK(mp_elf_lit, "No .lit section in multiplayer ELF, it is either packed or not a R&C boot ELF.");
		
		mp_overlay_start_address = mp_elf_lit->addr;
		
		fill_in_section_symbols(table, input->mp_elf);
		for (u32 i = 0; i < table->symbol_count; i++)
		{
			Symbol* symbol = &table->symbols[i];
			if (!symbol->used)
				continue;
			
			if (symbol->temp_address >= mp_overlay_start_address)
				symbol->mpfrontend_address = symbol->temp_address;
			else
				symbol->mpcore_address = symbol->temp_address;
		}
	}
	
	if(input->frontbin_elf.data)
	{
		fill_in_section_symbols(table, input->frontbin_elf);
		for (u32 i = 0; i < table->symbol_count; i++)
		{
			Symbol* symbol = &table->symbols[i];
			if (!symbol->used)
				continue;
			
			symbol->frontbin_address = symbol->temp_address;
		}
	}
	
	for (u32 i = 0; i < input->overlay_count; i++)
	{
		CHECK(input->overlay_level_numbers[i] < table->level_count, "Level number from directory name out of bounds.");
		u32 overlay_index = table->levels[input->overlay_level_numbers[i]];
		
		fill_in_section_symbols(table, input->overlays[i]);
		for (u32 i = 0; i < table->symbol_count; i++)
		{
			Symbol* symbol = &table->symbols[i];
			if (!symbol->used)
				continue;
			
			symbol->overlay_addresses[overlay_index] = symbol->temp_address;
		}
	}
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

static void dump_vtbl(
	SymbolTable* table,
	Input* input,
	const char* section_name,
	u32 stride,
	const char** function_names,
	u32 function_count)
{
	b8* classes = malloc(MAX_CLASSES);
	memset(classes, 0, MAX_CLASSES);
	enumerate_classes(classes, input->boot_elf, section_name, stride);
	enumerate_classes(classes, input->mp_elf, section_name, stride);
	enumerate_classes(classes, input->frontbin_elf, section_name, stride);
	for (u32 i = 0; i < input->overlay_count; i++)
		enumerate_classes(classes, input->overlays[i], section_name, stride);
	
	u32 class_count = 0;
	for (u32 i = 0; i < MAX_CLASSES; i++)
		if (classes[i])
			class_count++;
	
	table->symbols = malloc(class_count * function_count * sizeof(Symbol));
	table->symbol_count = class_count * function_count;
	
	memset(table->symbols, 0, class_count * function_count * sizeof(Symbol));
	
	u32 symbol_index = 0;
	for (u32 i = 0; i < MAX_CLASSES; i++)
	{
		if (!classes[i])
			continue;
		
		for (u32 j = 0; j < function_count; j++)
			table->symbols[symbol_index++].name = format_class_symbol_name(function_names[j], i);
	}
	
	fill_in_class_symbols(table, input->boot_elf, section_name, stride, function_names, function_count);
	for (u32 i = 0; i < table->symbol_count; i++)
	{
		Symbol* symbol = &table->symbols[i];
		if (!symbol->used)
			continue;
		
		if (input->mp_elf.data)
			symbol->spfrontend_address = symbol->temp_address;
		else
			symbol->frontend_address = symbol->temp_address;
	}
	
	if(input->mp_elf.data)
	{
		fill_in_class_symbols(table, input->mp_elf, section_name, stride, function_names, function_count);
		for (u32 i = 0; i < table->symbol_count; i++)
		{
			Symbol* symbol = &table->symbols[i];
			if (!symbol->used)
				continue;
			
			symbol->mpfrontend_address = symbol->temp_address;
		}
	}
	
	if(input->frontbin_elf.data)
	{
		fill_in_class_symbols(table, input->frontbin_elf, section_name, stride, function_names, function_count);
		for (u32 i = 0; i < table->symbol_count; i++)
		{
			Symbol* symbol = &table->symbols[i];
			if (!symbol->used)
				continue;
			
			symbol->frontbin_address = symbol->temp_address;
		}
	}
	
	for (u32 i = 0; i < input->overlay_count; i++)
	{
		CHECK(input->overlay_level_numbers[i] < table->level_count, "Level number from directory name out of bounds.");
		u32 overlay_index = table->levels[input->overlay_level_numbers[i]];
		
		fill_in_class_symbols(table, input->overlays[i], section_name, stride, function_names, function_count);
		for (u32 i = 0; i < table->symbol_count; i++)
		{
			Symbol* symbol = &table->symbols[i];
			if (!symbol->used)
				continue;
			
			symbol->overlay_addresses[overlay_index] = symbol->temp_address;
		}
	}
}

static void enumerate_classes(b8 classes[MAX_CLASSES], Buffer elf, const char* section_name, u32 stride)
{
	if (!elf.data)
		return;
	
	ElfSectionHeader* section_header = lookup_section(elf, section_name);
	for (u32 offset = 0; offset < section_header->size; offset += stride)
	{
		u32* class_number = buffer_get(elf, section_header->offset + offset, 4, section_name);
		if (*class_number == UINT32_MAX)
			break;
		
		CHECK(*class_number < MAX_CLASSES, "Class number too high.");
		classes[*class_number] = TRUE;
	}
}

static void fill_in_class_symbols(
	SymbolTable* table,
	Buffer elf,
	const char* section_name,
	u32 stride,
	const char** function_names,
	u32 function_count)
{
	for (u32 i = 0; i < table->symbol_count; i++)
		table->symbols[i].used = FALSE;
	
	ElfSectionHeader* section_header = lookup_section(elf, section_name);
	for (u32 offset = 0; offset < section_header->size; offset += stride)
	{
		u32* class_number = buffer_get(elf, section_header->offset + offset, 4, section_name);
		if (*class_number == UINT32_MAX)
			break;
		
		for (u32 function = 0; function < function_count; function++)
		{
			u32* address = buffer_get(elf, section_header->offset + offset + (function + 1) * 4, 4, section_name);
			const char* name = format_class_symbol_name(function_names[function], *class_number);
			
			for (u32 symbol = 0; symbol < table->symbol_count; symbol++)
			{
				if (strcmp(table->symbols[symbol].name, name) == 0)
				{
					table->symbols[symbol].temp_address = *address;
					table->symbols[symbol].used = TRUE;
					break;
				}
			}
		}
	}
}

static const char* format_class_symbol_name(const char* function_name, u32 class_number)
{
	u32 name_size = strlen(function_name) + 8;
	char* name = malloc(name_size);
	snprintf(name, name_size, function_name, class_number);
	return name;
}

static void print_row(Symbol* symbol, Column columns[MAX_COLUMNS], u32 column_count, const char* symbol_type)
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
					printf("%s", symbol_type);
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

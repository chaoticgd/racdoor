/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <racdoor/buffer.h>
#include <racdoor/csv.h>
#include <racdoor/elf.h>
#include <racdoor/util.h>

#include <sys/stat.h>

#include <string.h>

static Buffer build_object_file(SymbolTable* table);

int main(int argc, char** argv)
{
	Buffer input_table = {};
	const char* serial = NULL;
	const char* output_directory = NULL;
	
	/* Parse the command line arguments. */
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-s") == 0)
		{
			i++;
			CHECK(i < argc, "Expected serial.");
			serial = argv[i];
		}
		else if (strcmp(argv[i], "-o") == 0)
		{
			i++;
			CHECK(i < argc, "Expected output path.");
			output_directory = argv[i];
		}
		else
		{
			CHECK(input_table.data == NULL);
			input_table = read_file(argv[i]);
		}
	}
	
	CHECK(input_table.data && serial && output_directory,
		"usage: %s <input table> -s <serial> -o <output directory>",
		(argc > 0) ? argv[0] : "inigen");
	
	/* Parse the input CSV file into a symbol table in memory. */
	SymbolTable table = parse_table(input_table);
	
	/* Lookup the PCSX2 CRC value for the game's bootable ELF file and the
	   address of the level number variable. */
	u32 pcsx2_crc = 0;
	u32 level_address = 0;
	for (u32 i = 0; i < table.symbol_count; i++)
	{
		if (strcmp(table.symbols[i].name, "_racdoor_pcsx2_crc") == 0)
			pcsx2_crc = table.symbols[i].core_address;
		
		if (strcmp(table.symbols[i].name, "Level") == 0)
			level_address = table.symbols[i].core_address;
	}
	
	CHECK(pcsx2_crc != 0, "No _racdoor_pcsx2_crc symbol.");
	CHECK(level_address != 0, "No Level symbol.");
	
	/* Generate the .ini file for PCSX2. */
	char ini_path[1024];
	snprintf(ini_path, sizeof(ini_path), "%s/%s_%08X.ini", output_directory, serial, pcsx2_crc);
	FILE* ini_file = fopen(ini_path, "w");
	
	fprintf(ini_file, "[Debugger/Analysis/ExtraSymbolFiles]\n");
	fprintf(ini_file, "Count = %d\n", table.level_count + 5);
	
	fprintf(ini_file, "\n");
	fprintf(ini_file, "[Debugger/Analysis/ExtraSymbolFiles/0]\n");
	fprintf(ini_file, "Path = %s/core.elf\n", serial);
	
	fprintf(ini_file, "\n");
	fprintf(ini_file, "[Debugger/Analysis/ExtraSymbolFiles/1]\n");
	fprintf(ini_file, "Path = %s/spcore.elf\n", serial);
	fprintf(ini_file, "Condition = 0\n"); /* TODO */
	
	fprintf(ini_file, "\n");
	fprintf(ini_file, "[Debugger/Analysis/ExtraSymbolFiles/2]\n");
	fprintf(ini_file, "Path = %s/mpcore.elf\n", serial);
	fprintf(ini_file, "Condition = 0\n"); /* TODO */
	
	fprintf(ini_file, "\n");
	fprintf(ini_file, "[Debugger/Analysis/ExtraSymbolFiles/3]\n");
	fprintf(ini_file, "Path = %s/frontend.elf\n", serial);
	fprintf(ini_file, "Condition = 0\n"); /* TODO */
	
	fprintf(ini_file, "\n");
	fprintf(ini_file, "[Debugger/Analysis/ExtraSymbolFiles/4]\n");
	fprintf(ini_file, "Path = %s/frontbin.elf\n", serial);
	fprintf(ini_file, "Condition = 0\n"); /* TODO */
	
	for (u32 i = 0; i < table.level_count; i++)
	{
		fprintf(ini_file, "\n");
		fprintf(ini_file, "[Debugger/Analysis/ExtraSymbolFiles/%d]\n", i + 5);
		fprintf(ini_file, "Path = %s/ovl%hhu.elf\n", serial, table.levels[i]);
		fprintf(ini_file, "Condition = [%x]==%x\n", level_address, i);
	}
	
	/* Create a directory for the .elf files. */
	char directory_path[1024];
	snprintf(directory_path, sizeof(directory_path), "%s/%s", output_directory, serial);
	mkdir(directory_path, 0777);
	
	/* Generate the .elf files referenced by the .ini file. */
	for (u32 i = 0; i < table.symbol_count; i++)
	{
		table.symbols[i].used = table.symbols[i].core_address != 0 && strncmp(table.symbols[i].name, "_racdoor", 8) != 0;
		table.symbols[i].temp_address = table.symbols[i].core_address;
	}
	
	Buffer core = build_object_file(&table);
	
	char core_path[1024];
	snprintf(core_path, sizeof(core_path), "%s/%s/core.elf", output_directory, serial);
	write_file(core_path, core);
	
	for (u32 i = 0; i < table.overlay_count; i++)
	{
		for (u32 j = 0; j < table.symbol_count; j++)
		{
			table.symbols[j].used = table.symbols[j].overlay_addresses[i] != 0 && strncmp(table.symbols[i].name, "_racdoor", 8) != 0;
			table.symbols[j].temp_address = table.symbols[j].overlay_addresses[i];
		}
		
		Buffer overlay = build_object_file(&table);
		
		char overlay_path[1024];
		snprintf(overlay_path, sizeof(overlay_path), "%s/%s/ovl%hhu.elf", output_directory, serial, (u8) i);
		write_file(overlay_path, overlay);
	}
}

static Buffer build_object_file(SymbolTable* table)
{
	/* Count the number of symbols to include in the ELF symbol table. */
	u32 symbol_count = 0;
	for (u32 i = 0; i < table->symbol_count; i++)
		if (table->symbols[i].used)
			symbol_count++;
	
	static const char* section_names[] = {
		"",
		".shstrtab",
		".symtab",
		".strtab"
	};
	
	/* Determine the layout of the object file that is to be written out. */
	u32 file_header_size = sizeof(ElfFileHeader);
	u32 shstrtab_size = 0;
	for (u32 i = 0; i < ARRAY_SIZE(section_names); i++)
		shstrtab_size += strlen(section_names[i]) + 1;
	u32 section_headers_size = ARRAY_SIZE(section_names) * sizeof(ElfSectionHeader);
	u32 symtab_size = (1 + symbol_count) * sizeof(ElfSymbol);
	u32 strtab_size = 1;
	for (u32 i = 0; i < table->symbol_count; i++)
		if (table->symbols[i].used)
			strtab_size += strlen(table->symbols[i].name) + 1;
	
	u32 file_header_offset = 0;
	u32 shstrtab_offset = ALIGN(file_header_offset + file_header_size, 4);
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
	header->shstrndx = 1;
	
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
			.link = 3,
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
	
	/* Fill in the symbol table. */
	ElfSymbol* symtab = (ElfSymbol*) &buffer.data[symtab_offset];
	char* strtab = &buffer.data[strtab_offset];
	
	ElfSymbol* symbol = symtab + 1;
	u32 name_offset = 1;
	
	for (u32 i = 0; i < table->symbol_count; i++)
	{
		if (table->symbols[i].used)
		{
			symbol->name = name_offset;
			symbol->value = table->symbols[i].temp_address;
			if (table->symbols[i].type == STT_OBJECT)
				symbol->size = table->symbols[i].size ? table->symbols[i].size : 1;
			else
				symbol->size = table->symbols[i].size;
			symbol->info = table->symbols[i].type | (STB_GLOBAL << 4);
			symbol->shndx = 0;
			
			strcpy(strtab + symbol->name, table->symbols[i].name);
			
			name_offset += strlen(table->symbols[i].name) + 1;
			symbol++;
		}
	}
	
	return buffer;
}

/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <racdoor/elf.h>

#ifdef _HOST

#include <string.h>

ElfFileHeader* parse_elf_header(Buffer object)
{
	ElfFileHeader* elf_header = buffer_get(object, 0, sizeof(ElfFileHeader), "ELF file header");
	CHECK(elf_header->ident_magic == 0x464c457f, "ELF file has bad magic number.");
	CHECK(elf_header->ident_class == 1, "ELF file isn't 32 bit.");
	CHECK(elf_header->machine == 8, "ELF file isn't compiled for MIPS.");
	return elf_header;
}

ElfSectionHeader* lookup_section(Buffer object, const char* section)
{
	ElfFileHeader* header = parse_elf_header(object);
	u32 shstrtab_offset = header->shoff + header->shstrndx * sizeof(ElfSectionHeader);
	ElfSectionHeader* shstrtab = buffer_get(object, shstrtab_offset, sizeof(ElfSectionHeader), "shstr section header");
	
	for (u32 i = 0; i < header->shnum; i++)
	{
		u32 section_offset = header->shoff + i * sizeof(ElfSectionHeader);
		ElfSectionHeader* section_header = buffer_get(object, section_offset, sizeof(ElfSectionHeader), "section header");
		
		const char* name = buffer_string(object, shstrtab->offset + section_header->name, "section name");
		if (strcmp(name, section) == 0)
			return section_header;
	}
	
	ERROR("No '%s' section.", section);
}

u32 lookup_symbol(Buffer object, const char* symbol)
{
	ElfFileHeader* header = parse_elf_header(object);
	ElfSectionHeader* symtab = lookup_section(object, ".symtab");
	
	/* Find the string table section. */
	ElfSectionHeader* strtab = NULL;
	if (symtab->link != 0)
	{
		u32 link_offset = header->shoff + symtab->link * sizeof(ElfSectionHeader);
		strtab = buffer_get(object, link_offset, sizeof(ElfSectionHeader), "linked section header");
	}
	
	CHECK(strtab, "No linked string table section.");
	
	/* Lookup the symbol by name. */
	ElfSymbol* symbols = buffer_get(object, symtab->offset, symtab->size, "symbol table");
	for (u32 i = 0; i < symtab->size / sizeof(ElfSymbol); i++)
	{
		const char* name = buffer_string(object, strtab->offset + symbols[i].name, "symbol name");
		if (strcmp(name, symbol) == 0)
		{
			printf("%s: %x\n", symbol, symbols[i].value);
			return symbols[i].value;
		}
	}
	
	ERROR("Undefined symbol '%s'.", symbol);
}

u32 lookup_runtime_symbol_index(Buffer symbolmap, const char* name)
{
	RacdoorSymbolMapHead* head = buffer_get(symbolmap, 0, sizeof(RacdoorSymbolMapHead), "symbol map header");
	
	for (u32 i = 0; i < head->symbol_count; i++)
	{
		const char* string = buffer_string(symbolmap, head->entries[i].string_offset, "symbol name");
		if (strcmp(string, name) != 0)
			continue;
		
		return head->entries[i].runtime_index;
	}
	
	ERROR("Undefined symbol '%s'.", name);
}

#endif

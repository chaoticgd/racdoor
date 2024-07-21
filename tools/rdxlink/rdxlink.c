/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <racdoor/buffer.h>
#include <racdoor/elf.h>
#include <racdoor/linker.h>
#include <racdoor/util.h>

#include <string.h>

typedef struct {
	ElfSectionHeader* header;
	Buffer buffer;
} ElfSection;

typedef struct {
	/* The relocations. */
	ElfRelocation* relocations;
	u32 relocation_count;
	
	/* Linked sections. */
	ElfSection target;
	ElfSection symtab;
	ElfSection strtab;
} RelocationSection;

#define MAX_RELOC_SECTIONS 32

typedef struct {
	ElfSectionHeader* headers;
	u32 count;
	
	/* Input relocation sections. */
	RelocationSection reloc_sections[MAX_RELOC_SECTIONS];
	u32 reloc_section_count;
	
	/* Output relocation section. */
	ElfSection racdoor_relocs;
	
	/* Maps symbol strings to runtime indices. */
	ElfSection racdoor_symbolmap;
} ElfSections;

ElfSections parse_elf_sections(Buffer elf, ElfFileHeader* header);
void process_relocations(ElfSections* sections);

int main(int argc, char** argv)
{
	CHECK(argc == 3, "usage: %s <input elf> <output rdx>", (argc > 0) ? argv[0] : "rdxprep");
	
	const char* input_elf_path = argv[1];
	const char* output_rdx_path = argv[2];
	
	Buffer elf = read_file(input_elf_path);
	
	ElfFileHeader* elf_header = parse_elf_header(elf);
	ElfSections sections = parse_elf_sections(elf, elf_header);
	process_relocations(&sections);
	
	write_file(output_rdx_path, elf);
}

ElfSections parse_elf_sections(Buffer elf, ElfFileHeader* header)
{
	ElfSections result = {};
	
	ElfSectionHeader* sections = buffer_get(elf, header->shoff, header->shnum * sizeof(ElfSectionHeader), "section headers");
	CHECK(header->shstrndx < header->shnum, "Invalid shstrndx.\n");
	
	for (u32 i = 0; i < header->shnum; i++)
	{
		if (sections[i].type == SHT_REL)
		{
			/* Enumerate all the relocation sections so that we can apply
			   static relocations below, and copy dynamic relocations to be
			   applied at runtime. */
			
			CHECK(result.reloc_section_count < MAX_RELOC_SECTIONS, "Too many SHT_REL sections.\n");
			RelocationSection* output = &result.reloc_sections[result.reloc_section_count++];
			
			output->relocations = buffer_get(elf, sections[i].offset, sections[i].size, "relocations");
			output->relocation_count = sections[i].size / sizeof(ElfRelocation);
			
			CHECK(sections[i].info < header->shnum, "Relocation section has invalid info field.\n");
			ElfSectionHeader* target = &sections[sections[i].info];
			output->target.header = target;
			output->target.buffer = sub_buffer(elf, target->offset, target->size, "section to be relocated");
			
			CHECK(sections[i].link < header->shnum, "Relocation section has invalid link field.\n");
			ElfSectionHeader* symtab = &sections[sections[i].link];
			output->symtab.header = symtab;
			output->symtab.buffer = sub_buffer(elf, symtab->offset, symtab->size, "linked symbol table section");
			
			CHECK(symtab->link < header->shnum, "Symbol table section has invalid link field.\n");
			ElfSectionHeader* strtab = &sections[symtab->link];
			output->strtab.header = strtab;
			output->strtab.buffer = sub_buffer(elf, strtab->offset, strtab->size, "linked string table section");
		}
		else
		{
			const char* name = buffer_string(elf, sections[header->shstrndx].offset + sections[i].name, "section name");
			
			ElfSection* output = NULL;
			if (strcmp(name, ".racdoor.relocs") == 0)
				output = &result.racdoor_relocs;
			else if (strcmp(name, ".racdoor.symbolmap") == 0)
				output = &result.racdoor_symbolmap;
			
			if (!output)
				continue;
			
			output->header = &sections[i];
			output->buffer = sub_buffer(elf, sections[i].offset, sections[i].size, "section data");
		}
	}
	
	result.headers = sections;
	result.count = header->shnum;
	
	return result;
}

void process_relocations(ElfSections* sections)
{
	RacdoorRelocation* reloc_out = (RacdoorRelocation*) sections->racdoor_relocs.buffer.data;
	RacdoorRelocation* reloc_end = (RacdoorRelocation*) (sections->racdoor_relocs.buffer.data + sections->racdoor_relocs.buffer.size);
	
	CHECK(reloc_out, "Missing .racdoor.relocs section.\n");
	
	u32 dynamic_relocation_count = 0;
	
	/* Iterate over all the SHT_REL sections e.g. .rel.text, .rel.data, etc. */
	for (u32 i = 0; i < sections->reloc_section_count; i++)
	{
		RelocationSection* section = &sections->reloc_sections[i];
		
		for (u32 j = 0; j < section->relocation_count; j++)
		{
			ElfRelocation* reloc_in = &section->relocations[j];
		
			u8 type = reloc_in->info & 0xff;
			u32 symbol_index = reloc_in->info >> 8;
			
			ElfSymbol* symbol = buffer_get(section->symtab.buffer, symbol_index * sizeof(ElfSymbol), sizeof(ElfSymbol), "symbol");
			
			if (symbol->shndx > 0)
			{
				/* This is a static relocation which is to be applied at build
				   time, but because we've instructed GNU ld to only do a
				   partial link, it hasn't been applied yet. Hence, we apply
				   the relocation now. */
				
				CHECK(symbol->shndx < sections->count, "Symbol has invalid shndx field.\n");
				ElfSectionHeader* symbol_section = &sections->headers[symbol->shndx];
				
				u32* dest = buffer_get(section->target.buffer, reloc_in->offset, 4, "relocation target");
				apply_relocation(dest, type, symbol_section->addr + symbol->value);
			}
			else
			{
				/* This is a dynamic relocation that depends on the currently
				   loaded level overlay. We convert the offset to an absolute
				   address and write it out into the .racdoor.relocs section so
				   that it can be applied ar runtime. */
				
				const char* name = buffer_string(section->strtab.buffer, symbol->name, "symbol string");
				
				/* The index of the symbol in the ELF .symtab section has to be
				   converted to the equivalent .racdoor.addrtbl index. */
				u32 runtime_index = lookup_runtime_symbol_index(sections->racdoor_symbolmap.buffer, name);
				
				CHECK(reloc_out < reloc_end, "Not enough space for relocations.\n");
				reloc_out->address = section->target.header->addr + reloc_in->offset;
				reloc_out->info = type | (runtime_index << 8);
				reloc_out++;
				
				/* Generate an error if the relocation is of a type that we
				   can't handle. */
				u32 dummy;
				apply_relocation(&dummy, type, 0);
				
				dynamic_relocation_count++;
			}
		}
	}
	
	CHECK(reloc_out < reloc_end, "Not enough space for relocations.\n");
	reloc_out->address = 0xffffffff;
	reloc_out->info = 0;
	
	sections->racdoor_relocs.header->size = dynamic_relocation_count * sizeof(RacdoorRelocation) + 4;
}

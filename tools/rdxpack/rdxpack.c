/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <racdoor/elf.h>
#include <racdoor/util.h>

#include <string.h>

#define MAX_OUTPUT_SIZE (32 * 1024 * 1024)

typedef enum {
	SLT_IGNORE,
	SLT_COPY,
	SLT_FILL,
	SLT_DECOMPRESS
} SectionLoadType;

typedef struct {
	SectionLoadType type;
	Buffer compressed;
} SectionWork;

void pack_symbols(RacdoorSymbolHeader* symbols, Buffer elf);
u32 pack_payload(RacdoorFileHeader* file_header, Buffer payload, Buffer elf, u32 payload_address, int enable_compression);

/* From compression.cpp. */
void compress_wad(char** dest, unsigned int* dest_size, const char* src, unsigned int src_size, const char* muffin);

int main(int argc, char** argv)
{
	const char* input_elf_path = NULL;
	const char* output_rdx_path = NULL;
	int enable_compression = 1;
	
	/* Parse the command line arguments. */
	u32 position = 0;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-u") == 0)
		{
			enable_compression = 0;
		}
		else
		{
			switch (position++)
			{
				case 0: input_elf_path = argv[i]; break;
				case 1: output_rdx_path = argv[i]; break;
				default: input_elf_path = NULL; /* Print usage message. */
			}
		}
	}
	
	CHECK(input_elf_path && output_rdx_path,
		"usage: %s [-u] <input elf> <output rdx>\n",
		argc > 0 ? argv[0] : "rdxpack");
	
	Buffer elf = read_file(input_elf_path);
	Buffer rdx = {
		checked_malloc(MAX_OUTPUT_SIZE),
		MAX_OUTPUT_SIZE
	};
	memset(rdx.data, 0xee, MAX_OUTPUT_SIZE);
	
	u32 offset = 0;
	
	RacdoorFileHeader* header = (RacdoorFileHeader*) (rdx.data + offset);
	offset += sizeof(RacdoorFileHeader);
	header->magic = FOURCC("RDX!");
	header->version = RDX_FORMAT_VERSION;
	
	/* Fill in the serial an muffin. */
	size_t muffin_size = MIN(strlen(input_elf_path), sizeof(header->muffin));
	memcpy(header->muffin, input_elf_path, muffin_size);
	if (muffin_size < sizeof(header->muffin))
		header->muffin[muffin_size] = '\0';
	
	ElfSectionHeader* serial_section = lookup_section(elf, ".racdoor.serial");
	const char* serial = buffer_string(elf, serial_section->offset, "serial number");
	size_t serial_size = MIN(strlen(serial), sizeof(header->serial));
	memcpy(header->serial, serial, serial_size);
	if (serial_size < sizeof(header->serial))
		header->serial[serial_size] = '\0';
	
	/* Pack the exploit symbols into the header. */
	pack_symbols(&header->symbols, elf);
	
	offset = ALIGN(offset, 64);
	
	/* Convert the ELF file into an RDX payload. */
	Buffer payload = sub_buffer(rdx, offset, MAX_OUTPUT_SIZE - offset, "payload");
	
	header->payload_ofs = offset;
	header->payload_size = pack_payload(header, payload, elf, header->symbols.payload, enable_compression);
	rdx.size = header->payload_ofs + header->payload_size;
	
	write_file(output_rdx_path, rdx);
}

void pack_symbols(RacdoorSymbolHeader* symbols, Buffer elf)
{
	symbols->max_level = lookup_symbol(elf, "_racdoor_max_level");
	symbols->help_message = lookup_symbol(elf, "_racdoor_help_message");
	symbols->help_gadget = lookup_symbol(elf, "_racdoor_help_gadget");
	symbols->help_log = lookup_symbol(elf, "_racdoor_help_log");
	symbols->initial_hook = lookup_symbol(elf, "_racdoor_initial_hook");
	symbols->return_to_game = lookup_symbol(elf, "_racdoor_return_to_game");
	symbols->original_instruction = lookup_symbol(elf, "_racdoor_original_instruction");
	symbols->trampoline = lookup_symbol(elf, "_racdoor_trampoline");
	symbols->trampoline_offset = lookup_symbol(elf, "_racdoor_trampoline_offset");
	symbols->trampoline_block = lookup_symbol(elf, "_racdoor_trampoline_block");
	symbols->decryptor = lookup_symbol(elf, "_racdoor_decryptor");
	symbols->decryptor_block = lookup_symbol(elf, "_racdoor_decryptor_block");
	symbols->payload = lookup_symbol(elf, "_racdoor_payload");
	symbols->payload_end = lookup_symbol(elf, "_racdoor_payload_end");
	symbols->payload_block = lookup_symbol(elf, "_racdoor_payload_block");
	symbols->modload_hook_ofs = lookup_symbol(elf, "_racdoor_modload_hook_ofs");
	symbols->modupdate_hook_ofs = lookup_symbol(elf, "_racdoor_modupdate_hook_ofs");
	symbols->modunload_hook_ofs = lookup_symbol(elf, "_racdoor_modunload_hook_ofs");
}

u32 pack_payload(RacdoorFileHeader* file_header, Buffer payload, Buffer elf, u32 payload_address, int enable_compression)
{
	u32 offset = 0;
	
	printf("%.2fkb total\n", payload.size / 1024.f);
	
	CHECK((u64) offset + sizeof(RacdoorPayloadHeader) <= payload.size, "RDX too big!\n");
	RacdoorPayloadHeader* payload_header = (RacdoorPayloadHeader*) &payload.data[offset];
	offset += sizeof(RacdoorPayloadHeader);
	
	memset(payload_header, 0, sizeof(RacdoorPayloadHeader));
	
	ElfFileHeader* elf_header = parse_elf_header(elf);
	
	ElfSectionHeader* sections = buffer_get(elf, elf_header->shoff, elf_header->shnum * sizeof(ElfSectionHeader), "RDX section headers");
	
	/* Determine how each of the sections in the file should be loaded. */
	SectionWork* work = checked_malloc(elf_header->shnum * sizeof(SectionWork));
	for (u32 i = 0; i < elf_header->shnum; i++)
	{
		/* Skip over sections that shouldn't be included in the output e.g. the
		  symbol table and the string table. */
		if (sections[i].addr == 0 || sections[i].size == 0)
		{
			work[i].type = SLT_IGNORE;
			continue;
		}
		
		/* The .bss section is usually marked SHT_NOBITS. */
		if (sections[i].type == SHT_NOBITS)
		{
			work[i].type = SLT_FILL;
			continue;
		}
		
		int is_compressable = enable_compression;
		
		/* Test for special section names indicating that the section can't be
		   compressed. */
		if (is_compressable)
		{
			static const char* uncompressable_sections[] = {
				".racdoor.loader", /* The loader can't be compressed. */
				".racdoor.overlaymap", /* This section is used by the loader. */
				".racdoor.fastdecompress" /* This section is used by the loader. */
			};
			
			const char* name = buffer_string(elf, sections[elf_header->shstrndx].offset + sections[i].name, "section name");
			
			for (u32 j = 0; j < ARRAY_SIZE(uncompressable_sections); j++)
			{
				if (strcmp(uncompressable_sections[j], name) == 0)
				{
					is_compressable = 0;
					break;
				}
			}
		}
		
		/* Compress the section and see if the output is actually any smaller. */
		if (is_compressable)
		{
			u8* data = buffer_get(elf, sections[i].offset, sections[i].size, "RDX section");
			compress_wad(&work[i].compressed.data, &work[i].compressed.size, (const char*) data, sections[i].size, "");
			
			if (work[i].compressed.size >= sections[i].size)
				is_compressable = 0;
		}
		
		if (is_compressable)
			work[i].type = SLT_DECOMPRESS;
		else
			work[i].type = SLT_COPY;
	}
	
	/* Make room for the load headers. */
	for (u32 i = 0; i < elf_header->shnum; i++)
	{
		if (work[i].type == SLT_IGNORE)
			continue;
		
		CHECK((u64) offset + sizeof(RacdoorLoadHeader) <= payload.size, "RDX too big!\n");
		offset += sizeof(RacdoorLoadHeader);
	}
	
	u32 load_index = 0;
	u32 entry_point_offset = 0;
	
	/* Write out all the COPY sections. */
	for (u32 i = 0; i < elf_header->shnum; i++)
	{
		if (work[i].type != SLT_COPY)
			continue;
		
		/* Minimum alignment for instructions. */
		offset = ALIGN(offset, 4);
		
		if (elf_header->entry >= sections[i].addr && elf_header->entry < (sections[i].addr + sections[i].size))
			entry_point_offset = offset + (elf_header->entry - sections[i].addr);
		
		RacdoorLoadHeader* load_header = &payload_header->loads[load_index++];
		
		CHECK(offset <= 0xffff, "Cannot encode load offset.\n");
		CHECK(sections[i].size <= 0xffff, "Cannot encode load size.\n");
		load_header->dest = sections[i].addr;
		load_header->source = (u16) offset;
		load_header->size = (u16) sections[i].size;
		
		u8* data = buffer_get(elf, sections[i].offset, sections[i].size, "RDX section");
		CHECK((u64) offset + sections[i].size <= payload.size, "RDX too big!\n");
		memcpy(&payload.data[offset], data, sections[i].size);
		offset += sections[i].size;
		
		payload_header->copy_count++;
	}
	
	CHECK(entry_point_offset != 0, "Bad entry point.\n");
	file_header->entry = payload_address + entry_point_offset;
	
	/* Write out all the FILL sections. */
	for (u32 i = 0; i < elf_header->shnum; i++)
	{
		if (work[i].type != SLT_FILL)
			continue;
		
		RacdoorLoadHeader* load_header = &payload_header->loads[load_index++];
		
		CHECK(offset <= 0xffff, "Cannot encode load offset.\n");
		CHECK(sections[i].size <= 0xffff, "Cannot encode load size.\n");
		load_header->dest = sections[i].addr;
		load_header->source = 0; /* Fill value. */
		load_header->size = (u16) sections[i].size;
		
		payload_header->fill_count++;
	}
	
	/* Write out all the DECOMPRESS sections. */
	for (u32 i = 0; i < elf_header->shnum; i++)
	{
		if (work[i].type != SLT_DECOMPRESS)
			continue;
		
		RacdoorLoadHeader* load_header = &payload_header->loads[load_index++];
		
		/* Minimum alignment for DMA transfers. */
		offset = ALIGN(offset, 16);
		
		CHECK(offset <= 0xffff, "Cannot encode load offset.\n");
		CHECK(sections[i].size <= 0xffff, "Cannot encode load size.\n");
		load_header->dest = sections[i].addr;
		load_header->source = (u16) offset;
		load_header->size = (u16) 0; /* Unused. */
		
		CHECK((u64) offset + work[i].compressed.size <= payload.size, "RDX too big!\n");
		memcpy(&payload.data[offset], work[i].compressed.data, work[i].compressed.size);
		offset += work[i].compressed.size;
		
		payload_header->decompress_count++;
	}
	
	printf("%.2fkb used\n", offset / 1024.f);
	printf("%.2fkb left\n", (payload.size - offset) / 1024.f);
	
	return offset;
}

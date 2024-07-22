/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <racdoor/crypto.h>
#include <racdoor/elf.h>
#include <racdoor/exploit.h>
#include <racdoor/mips.h>
#include <racdoor/save.h>
#include <racdoor/util.h>

#include <string.h>
#include <time.h>

void inject_rac(SaveSlot* save, Buffer rdx, int enable_compression);
u32 pack_rdx(u8* output, u32 output_size, Buffer rdx, int enable_compression);

int main(int argc, char** argv)
{
	const char* input_save_path = NULL;
	const char* input_rdx_path = NULL;
	const char* output_save_path = NULL;
	int enable_compression = 1;
	
	u32 position = 0;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-u") == 0)
			enable_compression = 0;
		else
			switch (position++)
			{
				case 0: input_save_path = argv[i]; break;
				case 1: input_rdx_path = argv[i]; break;
				case 2: output_save_path = argv[i]; break;
				default: input_save_path = NULL; /* Print usage message. */
			}
	}
	
	CHECK(input_save_path && input_rdx_path && output_save_path,
		"usage: %s <input saveN.bin> <input rdx> <output saveN.bin>\n",
		argc > 0 ? argv[0] : "inject");
	
	Buffer file = read_file(input_save_path);
	SaveSlot save = parse_save(file);
	
	Buffer rdx = read_file(input_rdx_path);
	
	switch (save.game.block_count)
	{
		case RAC_GAME_BLOCK_COUNT:
			inject_rac(&save, rdx, enable_compression);
			break;
		case GC_GAME_BLOCK_COUNT:
			printf("not yet implemented\n");
			break;
		case UYA_GAME_BLOCK_COUNT:
			printf("not yet implemented\n");
			break;
		case DL_GAME_BLOCK_COUNT:
			printf("not yet implemented\n");
			break;
		default:
			ERROR("Unexpected block count.\n");
	}
	
	update_checksums(&save);
	write_file(output_save_path, file);
}

void inject_rac(SaveSlot* save, Buffer rdx, int enable_compression)
{
	/* Lookup special symbols used for configuring the exploit. */
	u32 help_message = lookup_symbol(rdx, "_racdoor_help_message");
	u32 help_gadget = lookup_symbol(rdx, "_racdoor_help_gadget");
	u32 help_log = lookup_symbol(rdx, "_racdoor_help_log");
	u32 initial_hook = lookup_symbol(rdx, "_racdoor_initial_hook");
	u32 trampoline_offset = lookup_symbol(rdx, "_racdoor_trampoline_offset");
	u32 trampoline_block = lookup_symbol(rdx, "_racdoor_trampoline_block");
	u32 decryptor = lookup_symbol(rdx, "_racdoor_decryptor");
	u32 decryptor_block = lookup_symbol(rdx, "_racdoor_decryptor_block");
	u32 payload = lookup_symbol(rdx, "_racdoor_payload");
	u32 payload_end = lookup_symbol(rdx, "_racdoor_payload_end");
	
	/* Pack and encrypt the payload image. */
	u32 block_size = 2048;
	u32 max_size = block_size * save->level_count;
	
	u8* data = checked_malloc(max_size);
	memset(data, 0, sizeof(max_size));
	
	u32 entry_offset = pack_rdx(data, max_size, rdx, enable_compression);
	u32 entry = payload + entry_offset;
	
	printf("entry: %x\n", entry);
	
	srand(time(NULL));
	u32 key = rand();
	
	printf("key: %x\n", key);
	
	xor_crypt((u32*) data, (u32*) (data + max_size), key);
	
	/* Gather together a bunch of variables that are needed to setup the initial
	   hook. This is all explained in the arm function itself. */
	ExploitParams params;
	
	SaveBlock* voice_on = lookup_block(&save->game, BLOCK_HelpVoiceOn);
	SaveBlock* text_on = lookup_block(&save->game, BLOCK_HelpTextOn);
	CHECK(voice_on->size == 1, "Incorrectly sized VoiceOn block.\n");
	CHECK(text_on->size == 1, "Incorrectly sized TextOn block.\n");
	params.help_voice_on = (char*) voice_on->data;
	params.help_text_on = (char*) text_on->data;
	
	SaveBlock* help_data_messages = lookup_block(&save->game, BLOCK_HelpDataMessages);
	params.help_data_messages = (HelpDatum*) help_data_messages->data;
	params.help_data_messages_count = help_data_messages->size / sizeof(HelpDatum);
	params.enabled_help_message = help_message;
	
	SaveBlock* help_data_gadgets = lookup_block(&save->game, BLOCK_HelpDataGadgets);
	params.help_data_gadgets = (HelpDatum*) help_data_gadgets->data;
	params.help_data_gadgets_count = help_data_gadgets->size / sizeof(HelpDatum);
	params.enabled_help_gadget = help_gadget;
	
	SaveBlock* help_data_misc = lookup_block(&save->game, BLOCK_HelpDataMisc);
	params.help_data_misc = (HelpDatum*) help_data_misc->data;
	params.help_data_misc_count = help_data_misc->size / sizeof(HelpDatum);
	
	SaveBlock* help_log_pos = lookup_block(&save->game, BLOCK_HelpLogPos);
	CHECK(help_log_pos->size == 4, "Incorrectly sized HelpLogPos block.\n");
	params.help_log_pos = (int*) help_log_pos->data;
	
	params.help_log = help_log;
	params.initial_hook = initial_hook;
	
	SaveBlock* trampoline_save = lookup_block(&save->game, trampoline_block);
	CHECK((u64) trampoline_offset + 8 <= trampoline_save->size, "Symbol _racdoor_target_offset is too big.\n");
	params.trampoline = (u32*) &trampoline_save->data[trampoline_offset];
	params.trampoline_target = decryptor + DECRYPTOR_ENTRY_OFFSET;
	
	/* Setup the initial hook. */
	arm(&params);
	
	/* Generate some assembly code to decrypt the payload. */
	u8 decryptor_asm[256];
	gen_xor_decryptor((u32*) decryptor_asm, payload, payload_end, entry, key);
	
	/* Pack the decryptor into the save file. */
	u64 decryptor_asm_offset = 0;
	for (u32 i = 0; i < save->level_count; i++)
	{
		SaveBlock* dest = lookup_block(&save->levels[i], decryptor_block);
		u32 copy_size = MIN(dest->size, sizeof(decryptor_asm) - decryptor_asm_offset);
		memcpy(dest->data, decryptor_asm + decryptor_asm_offset, copy_size);
		
		decryptor_asm_offset += copy_size;
	}
	
	/* Write the encrypted payload image into the save file. */
	for (u32 i = 0; i < save->level_count; i++)
	{
		SaveBlock* dest = lookup_block(&save->levels[i], BLOCK_MapMask);
		memcpy(dest->data, data + i * block_size, block_size);
		
		u32 offset = (u32) (dest->data - (char*) save->header);
		printf("Written payload block for level %d at 0x%x.\n", i, offset);
	}
}

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

/* From compression.cpp. */
void compress_wad(char** dest, unsigned int* dest_size, const char* src, unsigned int src_size, const char* muffin);

u32 pack_rdx(u8* output, u32 output_size, Buffer rdx, int enable_compression)
{
	u32 offset = 0;
	
	printf("%.2fkb total\n", output_size / 1024.f);
	
	CHECK((u64) offset + sizeof(RacdoorPayloadHeader) <= output_size, "RDX too big!\n");
	RacdoorPayloadHeader* payload_header = (RacdoorPayloadHeader*) &output[offset];
	offset += sizeof(RacdoorPayloadHeader);
	
	memset(payload_header, 0, sizeof(RacdoorPayloadHeader));
	
	ElfFileHeader* rdx_header = parse_elf_header(rdx);
	
	ElfSectionHeader* sections = buffer_get(rdx, rdx_header->shoff, rdx_header->shnum * sizeof(ElfSectionHeader), "RDX section headers");
	
	/* Determine how each of the sections in the file should be loaded. */
	SectionWork* work = checked_malloc(rdx_header->shnum * sizeof(SectionWork));
	for (u32 i = 0; i < rdx_header->shnum; i++)
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
			
			const char* name = buffer_string(rdx, sections[rdx_header->shstrndx].offset + sections[i].name, "section name");
			
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
			u8* data = buffer_get(rdx, sections[i].offset, sections[i].size, "RDX section");
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
	for (u32 i = 0; i < rdx_header->shnum; i++)
	{
		if (work[i].type == SLT_IGNORE)
			continue;
		
		CHECK((u64) offset + sizeof(RacdoorLoadHeader) <= output_size, "RDX too big!\n");
		offset += sizeof(RacdoorLoadHeader);
	}
	
	u32 load_index = 0;
	u32 entry_point_offset = 0;
	
	/* Write out all the COPY sections. */
	for (u32 i = 0; i < rdx_header->shnum; i++)
	{
		if (work[i].type != SLT_COPY)
			continue;
		
		/* Minimum alignment for instructions. */
		offset = ALIGN(offset, 4);
		
		if (rdx_header->entry >= sections[i].addr && rdx_header->entry < (sections[i].addr + sections[i].size))
			entry_point_offset = offset + (rdx_header->entry - sections[i].addr);
		
		RacdoorLoadHeader* load_header = &payload_header->loads[load_index++];
		
		CHECK(offset <= 0xffff, "Cannot encode load offset.\n");
		CHECK(sections[i].size <= 0xffff, "Cannot encode load size.\n");
		load_header->dest = sections[i].addr;
		load_header->source = (u16) offset;
		load_header->size = (u16) sections[i].size;
		
		u8* data = buffer_get(rdx, sections[i].offset, sections[i].size, "RDX section");
		CHECK((u64) offset + sections[i].size <= output_size, "RDX too big!\n");
		memcpy(&output[offset], data, sections[i].size);
		offset += sections[i].size;
		
		payload_header->copy_count++;
	}
	
	CHECK(entry_point_offset != 0, "Bad entry point.\n");
	
	/* Write out all the FILL sections. */
	for (u32 i = 0; i < rdx_header->shnum; i++)
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
	for (u32 i = 0; i < rdx_header->shnum; i++)
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
		
		CHECK((u64) offset + work[i].compressed.size <= output_size, "RDX too big!\n");
		memcpy(&output[offset], work[i].compressed.data, work[i].compressed.size);
		offset += work[i].compressed.size;
		
		payload_header->decompress_count++;
	}
	
	printf("%.2fkb used\n", offset / 1024.f);
	printf("%.2fkb left\n", (output_size - offset) / 1024.f);
	
	return entry_point_offset;
}

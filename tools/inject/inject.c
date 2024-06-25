#include <racdoor/crypto.h>
#include <racdoor/elf.h>
#include <racdoor/mips.h>
#include <racdoor/save.h>
#include <racdoor/util.h>

#include <string.h>
#include <time.h>

void inject_rac(SaveSlot* save, Buffer rdx, int enable_compression);
u32 pack_rdx(u8* output, u32 output_size, Buffer rdx, int enable_compression);

int main(int argc, char** argv)
{
	CHECK(argc == 4, "usage: %s <input saveN.bin> <input rdx> <output saveN.bin>\n",
		argc > 0 ? argv[0] : "inject");
	
	const char* input_save_path = argv[1];
	const char* input_rdx_path = argv[2];
	const char* output_save_path = argv[3];
	
	Buffer file = read_file(input_save_path);
	SaveSlot save = parse_save(file);
	
	Buffer rdx = read_file(input_rdx_path);
	
	switch (save.game.block_count)
	{
		case RAC_GAME_BLOCK_COUNT:
			inject_rac(&save, rdx, 1);
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

typedef struct {
	u16 flag;
	u16 unknown_2;
	u32 unknown_4;
} HelpDatum;

void inject_rac(SaveSlot* save, Buffer rdx, int enable_compression)
{
	/* Lookup special symbols used for configuring the exploit. */
	u32 help_message = lookup_symbol(rdx, "_racdoor_help_message");
	u32 help_gadget = lookup_symbol(rdx, "_racdoor_help_gadget");
	u32 help_log = lookup_symbol(rdx, "_racdoor_help_log");
	u32 initial_hook = lookup_symbol(rdx, "_racdoor_initial_hook");
	u32 trampoline = lookup_symbol(rdx, "_racdoor_trampoline");
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
	
	u32 entry_point_offset = pack_rdx(data, max_size, rdx, enable_compression);
	u32 entry_point = payload + entry_point_offset;
	
	printf("entry_point: %x\n", entry_point);
	
	srand(time(NULL));
	u32 key = rand();
	
	xor_crypt((u32*) data, (u32*) (data + max_size), key);
	
	/* Generate some assembly code to decrypt the payload. */
	u8 decryptor_asm[256];
	u32 decryptor_entry_offset = gen_xor_decryptor((u32*) decryptor_asm, payload, payload_end, key, entry_point);
	
	/* Enable help text so that our exploit runs, but disable the help audio
	   since playing the same help message again and again would otherwise get
	   quite repetitive. */
	SaveBlock* voice_on = lookup_block(&save->game, BLOCK_HelpVoiceOn);
	SaveBlock* text_on = lookup_block(&save->game, BLOCK_HelpTextOn);
	CHECK(voice_on->size == 1, "Incorrectly sized VoiceOn block.\n");
	CHECK(text_on->size == 1, "Incorrectly sized TextOn block.\n");
	*(u8*) voice_on->data = 0;
	*(u8*) text_on->data = 1;
	
	/* TODO: More than 8 items must be unlocked for the help message we want to
	   use to be run. */
	
	/* Disable all the help messages except for the one we want to use to
	   trigger the initial out of bounds write. */
	
	SaveBlock* help_data_messages = lookup_block(&save->game, BLOCK_HelpDataMessages);
	for (u32 i = 0; i < help_data_messages->size / sizeof(HelpDatum); i++)
	{
		HelpDatum* message = &((HelpDatum*) help_data_messages->data)[i];
		message->flag = (i == help_message) ? 0 : -1;
		message->unknown_2 = 0;
		message->unknown_4 = 0;
	}
	
	SaveBlock* help_data_gadgets = lookup_block(&save->game, BLOCK_HelpDataGadgets);
	for (u32 i = 0; i < help_data_gadgets->size / sizeof(HelpDatum); i++)
	{
		HelpDatum* message = &((HelpDatum*) help_data_gadgets->data)[i];
		message->flag = -1;
		message->unknown_2 = 0;
		message->unknown_4 = 0;
	}
	
	SaveBlock* help_data_misc = lookup_block(&save->game, BLOCK_HelpDataMisc);
	for (u32 i = 0; i < help_data_misc->size / sizeof(HelpDatum); i++)
	{
		HelpDatum* message = &((HelpDatum*) help_data_misc->data)[i];
		message->flag = (i == help_gadget) ? 0 : -1;
		message->unknown_2 = 0;
		message->unknown_4 = 0;
	}
	
	/* Adjust the HelpLogPos variable so that the help message index is written
	   over the high byte of the immediate field from a relative branch
	   instruction such as to make the processor jump into and start executing
	   code loaded from the memory card. */
	SaveBlock* help_log_pos = lookup_block(&save->game, BLOCK_HelpLogPos);
	CHECK(help_log_pos->size == 4, "Incorrectly sized HelpLogPos block.\n");
	*(u32*) help_log_pos->data = (initial_hook + 1) - help_log;
	
	/* Because we cannot choose exactly where in the memory card data the
	   processor will jump to, we setup a trampoline to jump to a more desirable
	   location. */
	SaveBlock* trampoline_save = lookup_block(&save->game, trampoline_block);
	CHECK((u64) trampoline_offset + 8 < trampoline_save->size, "Symbol _racdoor_target_offset is too big.\n");
	*(u32*) &trampoline_save->data[trampoline_offset + 0] = MIPS_J(decryptor + decryptor_entry_offset);
	*(u32*) &trampoline_save->data[trampoline_offset + 4] = MIPS_NOP();
	
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
		printf("Written block for level %d at 0x%x.\n", i, offset);
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
		
		int is_uncompressable = 0;
		
		/* Test for special section names indicating that the section can't be
		   compressed. */
		static const char* uncompressable_sections[] = {
			".racdoor.loader", /* The loader can't be compressed. */
			".racdoor.levelmap", /* This section is used by the loader. */
			".racdoor.fastdecompress" /* This section is used by the loader. */
		};
		
		const char* name = buffer_string(rdx, sections[rdx_header->shstrndx].offset + sections[i].name, "section name");
		
		int is_compressable = 1;
		for (u32 j = 0; j < ARRAY_SIZE(uncompressable_sections); j++)
		{
			if (strcmp(uncompressable_sections[j], name) == 0)
			{
				is_compressable = 0;
				break;
			}
		}
		
		/* Compress the section and see if the output is actually any smaller. */
		if (is_compressable)
		{
			u8* data = buffer_get(rdx, sections[i].offset, sections[i].size, "RDX section");
			compress_wad(&work[i].compressed.data, &work[i].compressed.size, data, sections[i].size, "");
			
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
	
	/* Copy the array of FastDecompress function pointers into the payload. */
	if (enable_compression)
	{
		ElfSectionHeader* fastdecompress_header = lookup_section(rdx, ".racdoor.fastdecompress");
		u32* fastdecompress_funcs = buffer_get(rdx, fastdecompress_header->offset, fastdecompress_header->size, "FastDecompress pointers");
		
		CHECK((u64) offset + fastdecompress_header->size <= output_size, "RDX too big!\n");
		memcpy(&output[offset], fastdecompress_funcs, fastdecompress_header->size);
		offset += fastdecompress_header->size;
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

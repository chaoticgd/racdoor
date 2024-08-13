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

void inject_rac(SaveSlot* save, Buffer rdx, u32 key);

int main(int argc, char** argv)
{
	const char* input_save_path = NULL;
	const char* input_rdx_path = NULL;
	const char* output_save_path = NULL;
	u32 key = 0;
	
	/* Parse the command line arguments. */
	u32 position = 0;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-k") == 0)
		{
			i++;
			CHECK(i < argc, "Expected key.\n");
			key = strtoll(argv[i], NULL, 0);
		}
		else
		{
			switch (position++)
			{
				case 0: input_save_path = argv[i]; break;
				case 1: input_rdx_path = argv[i]; break;
				case 2: output_save_path = argv[i]; break;
				default: input_save_path = NULL; /* Print usage message. */
			}
		}
	}
	
	CHECK(input_save_path && input_rdx_path && output_save_path,
		"usage: %s [-k <key>] <input saveN.bin> <input rdx> <output saveN.bin>\n",
		argc > 0 ? argv[0] : "inject");
	
	Buffer file = read_file(input_save_path);
	SaveSlot save = parse_save(file);
	
	Buffer rdx = read_file(input_rdx_path);
	
	switch (save.game.block_count)
	{
		case RAC_GAME_BLOCK_COUNT:
			inject_rac(&save, rdx, key);
			break;
		case GC_GAME_BLOCK_COUNT:
			printf("Going Commando support not yet implemented.\n");
			break;
		case UYA_GAME_BLOCK_COUNT:
			printf("Up Your Arsenal support not yet implemented.\n");
			break;
		case DL_GAME_BLOCK_COUNT:
			printf("Deadlocked support not yet implemented.\n");
			break;
		default:
			ERROR("Unexpected block count.\n");
	}
	
	update_checksums(&save);
	write_file(output_save_path, file);
}

void inject_rac(SaveSlot* save, Buffer rdx, u32 key)
{
	/* Make sure the player has more than 8 gadgets unlocked. */
	SaveBlock* unlocks = lookup_block(&save->game, BLOCK_Unlocks);
	
	u32 gadgets_unlocked = 0;
	for (u32 i = 0; i < unlocks->size; i++)
		if (unlocks->data[i])
			gadgets_unlocked++;
	
	CHECK(gadgets_unlocked > 8,
		"A save file with more than 8 quick select equipable gadgets unlocked is needed.\n");
	
	/* Validate the RDX header. */
	RacdoorFileHeader* header = buffer_get(rdx, 0, sizeof(RacdoorFileHeader), "RDX file header");
	CHECK(header->magic == FOURCC("RDX!"),
		"RDX file has bad magic number.\n");
	CHECK(header->version == RDX_FORMAT_VERSION,
		"RDX file has wrong version number (is %u, should be %u).\n",
		header->version, RDX_FORMAT_VERSION);
	
	Buffer payload = sub_buffer(rdx, header->payload_ofs, header->payload_size, "RDX payload");
	
	/* Prepare and encrypt the payload image. */
	u32 block_size = 2048;
	u32 max_size = block_size * save->level_count;
	
	u8* data = checked_malloc(max_size);
	memset(data, 0, max_size);
	
	CHECK(payload.size <= max_size, "RDX payload too big!\n");
	memcpy(data, payload.data, payload.size);
	
	printf("entry: %x\n", header->entry);
	
	if (key == 0)
	{
		srand(time(NULL));
		key = rand();
	}
	
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
	params.enabled_help_message = header->symbols.help_message;
	
	SaveBlock* help_data_gadgets = lookup_block(&save->game, BLOCK_HelpDataGadgets);
	params.help_data_gadgets = (HelpDatum*) help_data_gadgets->data;
	params.help_data_gadgets_count = help_data_gadgets->size / sizeof(HelpDatum);
	params.enabled_help_gadget = header->symbols.help_gadget;
	
	SaveBlock* help_data_misc = lookup_block(&save->game, BLOCK_HelpDataMisc);
	params.help_data_misc = (HelpDatum*) help_data_misc->data;
	params.help_data_misc_count = help_data_misc->size / sizeof(HelpDatum);
	
	SaveBlock* help_log_pos = lookup_block(&save->game, BLOCK_HelpLogPos);
	CHECK(help_log_pos->size == 4, "Incorrectly sized HelpLogPos block.\n");
	params.help_log_pos = (int*) help_log_pos->data;
	
	params.help_log = header->symbols.help_log;
	params.initial_hook = header->symbols.initial_hook;
	
	SaveBlock* trampoline_save = lookup_block(&save->game, header->symbols.trampoline_block);
	CHECK((u64) header->symbols.trampoline_offset + 8 <= trampoline_save->size, "Symbol _racdoor_target_offset is too big.\n");
	params.trampoline = (u32*) &trampoline_save->data[header->symbols.trampoline_offset];
	params.trampoline_target = header->symbols.decryptor + DECRYPTOR_ENTRY_OFFSET;
	
	/* Setup the initial hook. */
	arm(&params);
	
	/* Generate some assembly code to decrypt the payload. */
	u8 decryptor_asm[256];
	gen_xor_decryptor((u32*) decryptor_asm, header->symbols.payload, header->symbols.payload_end, header->entry, key);
	
	/* Pack the decryptor into the save file. */
	u64 decryptor_asm_offset = 0;
	for (u32 i = 0; i < save->level_count; i++)
	{
		SaveBlock* dest = lookup_block(&save->levels[i], header->symbols.decryptor_block);
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

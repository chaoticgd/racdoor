/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <racdoor/save.h>

#ifdef _HOST

SaveSlot parse_save(Buffer file)
{
	u32 offset = 0;
	
	SaveSlot save;
	
	save.header = buffer_get(file, offset, sizeof(SaveFileHeader), "file header");
	offset += sizeof(SaveFileHeader);
	
	save.game = parse_blocks(sub_buffer(file, offset, save.header->game_size, "game section"));
	offset += save.header->game_size;
	
	u32 level = 0;
	while (offset + 3 < file.size)
	{
		CHECK(level < MAX_LEVELS, "Too many levels.");
		
		save.levels[level] = parse_blocks(sub_buffer(file, offset, save.header->level_size, "level section"));
		offset += save.header->level_size;
		
		level++;
	}
	
	save.level_count = level;
	
	return save;
}

SaveSlotBlockList parse_blocks(Buffer file)
{
	u32 offset = 0;
	
	SaveSlotBlockList list;
	
	list.checksum = buffer_get(file, offset, sizeof(SaveChecksum), "checksum header");
	offset += sizeof(SaveChecksum);
	
	Buffer checksum_buffer = sub_buffer(file, offset, list.checksum->size, "checksum data");
	s32 checksum_value = compute_checksum(checksum_buffer.data, checksum_buffer.size);
	CHECK(checksum_value == list.checksum->value, "Bad checksum.");

	u32 block = 0;
	for (;;)
	{
		CHECK(block < MAX_BLOCKS, "Too many blocks.");
		
		list.blocks[block] = buffer_get(file, offset, sizeof(SaveBlock), "block header");
		offset += sizeof(SaveBlock);
		
		if(list.blocks[block]->type == -1)
			break;
		
		buffer_get(file, offset, list.blocks[block]->size, "block data");
		offset = ALIGN(offset + list.blocks[block]->size, 4);
		
		block++;
	}
	
	list.block_count = block;
	
	return list;
}

void update_checksums(SaveSlot* save)
{
	save->game.checksum->value = compute_checksum(save->game.checksum->data, save->game.checksum->size);
	for (u32 i = 0; i < save->level_count; i++)
		save->levels[i].checksum->value = compute_checksum(save->levels[i].checksum->data, save->levels[i].checksum->size);
}

s32 compute_checksum(const char* ptr, u32 size)
{
	u32 value = 0xedb88320;
	for (u32 i = 0; i < size; i++)
	{
		value ^= (u32) ptr[i] << 8;
		for (u32 repeat = 0; repeat < 8; repeat++)
		{
			if ((value & 0x8000) == 0)
				value <<= 1;
			else
				value = (value << 1) ^ 0x1f45;
		}
	}
	return (s32) (value & 0xffff);
}

SaveBlock* lookup_block(SaveSlotBlockList* list, s32 type)
{
	SaveBlock* block = NULL;
	for (u32 i = 0; i < list->block_count; i++)
		if (list->blocks[i]->type == type)
			block = list->blocks[i];
	
	CHECK(block, "No block of type %d found.", type);
	return block;
}

#endif

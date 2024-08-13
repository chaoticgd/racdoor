/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#ifndef _RACDOOR_SAVE_H
#define _RACDOOR_SAVE_H

#ifdef _HOST

#include <racdoor/buffer.h>
#include <racdoor/util.h>

enum SaveBlockType {
	BLOCK_Level = 0,
	BLOCK_Unlocks = 10,
	BLOCK_HelpDataMessages = 16,
	BLOCK_HelpDataGadgets = 17,
	BLOCK_HelpDataMisc = 18,
	BLOCK_HelpVoiceOn = 28,
	BLOCK_HelpTextOn = 29,
	BLOCK_HelpLog = 1010,
	BLOCK_HelpLogPos = 1011,
	BLOCK_MapMask = 3002,
	BLOCK_g_HeroGadgetBox = 7008
};

typedef struct {
	u32 game_size;
	u32 level_size;
} SaveFileHeader;

typedef struct {
	s32 type;
	u32 size;
	char data[];
} SaveBlock;

typedef struct {
	u32 size;
	s32 value;
	char data[];
} SaveChecksum;

#define MAX_BLOCKS 64
#define MAX_LEVELS 64

#define RAC_GAME_BLOCK_COUNT 47
#define GC_GAME_BLOCK_COUNT 37
#define UYA_GAME_BLOCK_COUNT 40
#define DL_GAME_BLOCK_COUNT 29

typedef struct {
	SaveChecksum* checksum;
	SaveBlock* blocks[MAX_BLOCKS];
	u32 block_count;
} SaveSlotBlockList;

typedef struct {
	SaveFileHeader* header;
	SaveSlotBlockList game;
	SaveSlotBlockList levels[MAX_LEVELS];
	u32 level_count;
} SaveSlot;

SaveSlot parse_save(Buffer file);
SaveSlotBlockList parse_blocks(Buffer file);
void update_checksums(SaveSlot* save);
s32 compute_checksum(const char* ptr, u32 size);
SaveBlock* lookup_block(SaveSlotBlockList* list, s32 type);

#endif

#endif

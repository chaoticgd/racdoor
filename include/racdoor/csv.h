/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#ifndef _RACDOOR_CSV_H
#define _RACDOOR_CSV_H

#ifdef _HOST

#include <racdoor/elf.h>

#define MAX_OVERLAYS 100

typedef struct {
	const char* name;
	const char* comment;
	ElfSymbolType type;
	u32 size;
	u32 core_address;
	u32 spcore_address;
	u32 mpcore_address;
	u32 frontend_address;
	u32 frontbin_address;
	u32 overlay_addresses[MAX_OVERLAYS];
	u32 highest_address;
	s32 runtime_index;
	b8 overlay;
	b8 used;
} Symbol;

#define MAX_LEVELS 100

typedef struct {
	u32 overlay_count;
	u8 levels[MAX_LEVELS];
	u32 level_count;
	Symbol* symbols;
	u32 symbol_count;
} SymbolTable;

SymbolTable parse_table(Buffer input);

#endif

#endif

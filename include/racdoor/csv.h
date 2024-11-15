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
	u32 spfrontend_address;
	u32 mpfrontend_address;
	u32 frontbin_address;
	u32 overlay_addresses[MAX_OVERLAYS];
	u32 temp_address;
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

typedef enum {
	COLUMN_NAME, /* Symbol name. This should be a non-mangled C identifier. */
	COLUMN_TYPE, /* ELF symbol type. */
	COLUMN_COMMENT, /* Ignored. */
	COLUMN_SIZE, /* Size in bytes. */
	COLUMN_CORE, /* Address in a core section from the ELF. These are always loaded. */
	COLUMN_SPCORE, /* UYA specific. Address in a core section from the singleplayer ELF. */
	COLUMN_MPCORE, /* UYA specific. Address in a core section from the multiplayer ELF. */
	COLUMN_FRONTEND, /* Address in the main menu overlay from the ELF. */
	COLUMN_FRONTBIN, /* Address in the main menu overlay from MISC.WAD. */
	COLUMN_FIRST_OVERLAY, /* Address in a level overlay from the LEVEL*.WAD files. */
	MAX_COLUMNS = 100
} Column;

SymbolTable parse_table(Buffer input);
u32 parse_table_header(const char** p, SymbolTable* table, Column* columns, b8 expect_newline);

#endif

#endif

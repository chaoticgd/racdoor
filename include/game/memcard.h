/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#ifndef _GAME_MEMCARD_H
#define _GAME_MEMCARD_H

typedef struct {
	void* addr;
	int size;
	int iff;
	int mark;
} mc_data;

void memcard_MakeWholeSave(char *addr);
int memcard_RestoreData(char* addr, int index, mc_data* mcd);
int memcard_Save(int force, int level);

void PackMapMask(char* dest);

#endif

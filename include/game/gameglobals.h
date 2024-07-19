/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#ifndef _GAME_GAMEGLOBALS_H
#define _GAME_GAMEGLOBALS_H

typedef enum {
	GAME_MODE_NONE = -2,
	GAME_MODE_DEBUG = -1,
	GAME_MODE_NORMAL = 0,
	GAME_MODE_MOVIE = 1,
	GAME_MODE_SCENE = 2,
	GAME_MODE_PAUSE = 3,
	GAME_MODE_FREEZE = 4,
	GAME_MODE_VENDOR = 5,
	GAME_MODE_SPACE = 6,
	GAME_MODE_PUZZLE = 7,
	GAME_MODE_WEAPON_UPGRADE = 8,
	GAME_MODE_CREDITS = 9,
	GAME_MODE_LOBBY = 10,
	GAME_MODE_FLYBY = 11,
	GAME_MODE_THERMAL = 12,
	GAME_MODE_PRE_LOBBY_MEMCARD_LOAD = 13,
	GAME_MODE_PRE_LOBBY = 14,
	GAME_MODE_WAIT_FOR_MPSTART = 15,
	GAME_MODE_EXEC_MP_MEMCARD_COMMAND = 16,
	GAME_MODE_IOP_DEBUG = 17,
	GAME_MODE_MAX = 18
} gameMode_t;

extern gameMode_t gameMode;

extern char HelpVoiceOn;
extern char HelpTextOn;
extern int HelpLogPos;

#endif

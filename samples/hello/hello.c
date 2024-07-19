/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <game/fonts.h>

#include <racdoor/hook.h>
#include <racdoor/module.h>

int DrawBoltCount(void* unknown);
int draw_bolt_count_thunk(void* unknown);
AUTO_HOOK(DrawBoltCount, draw_bolt_count_thunk, draw_bolt_count_trampoline, int);

int draw_bolt_count_thunk(void* unknown)
{
	FontPrintRight(200, 100, 0xff00ffff, "Hello, World!", 13);
	
	return draw_bolt_count_trampoline(unknown);
}

/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#ifndef _RACDOOR_MODULE_H
#define _RACDOOR_MODULE_H

typedef struct {
	void (*load)();
	void (*unload)();
} Module;

#define MODULE_LOAD_FUNC(func) \
	void (*_modloadfunc_##func)(void) __attribute__ ((section (".racdoor.modloadfuncs"))) = func;

#define MODULE_UPDATE_FUNC(func) \
	void (*_modupdatefunc_##func)(void) __attribute__ ((section (".racdoor.modupdatefuncs"))) = func;

#define MODULE_UNLOAD_FUNC(func) \
	void (*_modunloadfunc_##func)(void) __attribute__ ((section (".racdoor.modunloadfuncs"))) = func;

void install_module_hooks();

void load_modules();
void update_modules();
void unload_modules();

#endif

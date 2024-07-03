#include <game/bmain.h>

#include <racdoor/hook.h>
#include <racdoor/module.h>

static CallHook load_hook = {};
static CallHook update_hook = {};
static CallHook unload_hook = {};

/* All relative to the startlevel function. */
extern void _racdoor_modload_hook_ofs;
extern void _racdoor_modupdate_hook_ofs;
extern void _racdoor_modunload_hook_ofs;

static void load_thunk();
static void update_thunk();
static void unload_thunk();

typedef void VoidFunc(void);

/* Function pointers to be called just before we enter the main loop. */
extern VoidFunc* _racdoor_modloadfuncs;
extern VoidFunc* _racdoor_modloadfuncs_end;

/* Function pointers to be called on every iteration of the main loop. */
extern VoidFunc* _racdoor_modupdatefuncs;
extern VoidFunc* _racdoor_modupdatefuncs_end;

/* Function pointers to be called just after we break out of the main loop. */
extern VoidFunc* _racdoor_modunloadfuncs;
extern VoidFunc* _racdoor_modunloadfuncs_end;

void install_module_hooks()
{
	install_call_hook(&load_hook, (char*) startlevel + (u32) &_racdoor_modload_hook_ofs, load_thunk);
	install_call_hook(&update_hook, (char*) startlevel + (u32) &_racdoor_modupdate_hook_ofs, update_thunk);
	install_call_hook(&unload_hook, (char*) startlevel + (u32) &_racdoor_modunload_hook_ofs, unload_thunk);
}

static void load_thunk()
{
	load_modules();
	
	((void (*)(void)) load_hook.original_func)();
}

static void update_thunk()
{
	update_modules();
	
	((void (*)(void)) update_hook.original_func)();
}

static void unload_thunk()
{
	unload_modules();
	
	((void (*)(void)) unload_hook.original_func)();
}

 void load_modules()
 {
	for (VoidFunc** func = &_racdoor_modloadfuncs; func < &_racdoor_modloadfuncs_end; func++)
		(*func)();
}

void update_modules()
{
	for (VoidFunc** func = &_racdoor_modupdatefuncs; func < &_racdoor_modupdatefuncs_end; func++)
		(*func)();
}

void unload_modules()
{
	for (VoidFunc** func = &_racdoor_modunloadfuncs; func < &_racdoor_modunloadfuncs_end; func++)
		(*func)();
}

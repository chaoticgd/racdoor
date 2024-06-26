#include <racdoor/persistence.h>

#include <racdoor/elf.h>
#include <racdoor/hook.h>
#include <racdoor/linker.h>
#include <racdoor/loader.h>
#include <racdoor/module.h>

void* ParseBin();
void* parse_bin_thunk();
FuncHook parse_bin_hook = {};
TRAMPOLINE(parse_bin_trampoline, void*);

int LoadFrontData();
int load_front_data_thunk();
FuncHook load_front_data_hook = {};
TRAMPOLINE(load_front_data_trampoline, int);

void FlushCache(int mode);

extern RacdoorPayloadHeader _racdoor_payload;

void install_persistence_hooks()
{
	/* Level transition hook. */
	install_hook(&parse_bin_hook, ParseBin, parse_bin_thunk, parse_bin_trampoline);
	
	/* Quit game hook. */
	install_hook(&load_front_data_hook, LoadFrontData, load_front_data_thunk, load_front_data_trampoline);

	/* Save hook. */
	
	/* Load hook. */
}

void* parse_bin_thunk()
{
	unload_modules();
	uninstall_all_hooks();
	
	void* startlevel = parse_bin_trampoline();
	
	FlushCache(0);
	FlushCache(2);
	
	/* Unpack all the sections into memory once more, so that we can apply fresh
	   relocations for the new level to them. In addition, set the postload flag
	   so that the unpack routine knows to return instead of jumping back to the
	   initial loader code. */
	unpack();
	
	FlushCache(0);
	FlushCache(2);
	
	/* Apply the relocations for the new level. */
	apply_relocations();
	
	load_modules();
	
	return startlevel;
}

int load_front_data_thunk()
{
	unload_modules();
	uninstall_all_hooks();
	
	return load_front_data_trampoline();
}

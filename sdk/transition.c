#include <racdoor/transition.h>

#include <racdoor/elf.h>
#include <racdoor/hook.h>

FuncHook parse_bin_hook = {};
TRAMPOLINE(parse_bin_trampoline, void*);

void FlushCache(int mode);

void* ParseBin();
static void* parse_bin_thunk();

extern RacdoorPayloadHeader _racdoor_payload;

void unpack();
void apply_relocations();
void racdoor_entry();

void install_transition_hooks()
{
	install_hook(&parse_bin_hook, ParseBin, parse_bin_thunk, parse_bin_trampoline);
}

static void* parse_bin_thunk()
{
	u32 trampoline_backup[3];
	
	/* Backup the trampoline so that we can use it event after the below call to
	   uninstall_all_hooks, which would otherwise destroy it. */
	memcpy(trampoline_backup, parse_bin_trampoline, 12);
	
	uninstall_all_hooks();
	
	/* Copy the trampoline back. */
	memcpy(parse_bin_trampoline, trampoline_backup, 12);
	
	/* Make sure the trampoline is still bouncy. */
	FlushCache(0);
	FlushCache(2);
	
	void* startlevel = parse_bin_trampoline();
	
	/* Unpack all the sections into memory once more, so that we can apply fresh
	   relocations for the new level to them. In addition, set the postload flag
	   so that the unpack routine knows to return instead of jumping back to the
	   initial loader code. */
	_racdoor_payload.postload = 1;
	unpack();
	_racdoor_payload.postload = 0;
	
	FlushCache(0);
	FlushCache(2);
	
	/* Apply the relocations for the new level. */
	apply_relocations();
	
	racdoor_entry();
	
	return startlevel;
}

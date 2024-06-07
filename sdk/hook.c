#include <racdoor/hook.h>

#include <racdoor/mips.h>

static FuncHook* hook_head;

void install_hook(FuncHook* hook, void* original_func, void* replacement_func)
{
	/* Add to the list. */
	hook->prev = NULL;
	hook->next = hook_head;
	hook_head = hook;
	
	/* Backup original instructions. */
	hook->original_func = (u32*) original_func;
	hook->original_insns[0] = hook->original_func[0];
	hook->original_insns[1] = hook->original_func[1];
	
	/* Write new instructions. */
	hook->original_func[0] = MIPS_J((u32) replacement_func);
	hook->original_func[1] = MIPS_NOP();
	
	/* Flush the instruction cache. */
	FlushCache(2);
}

void uninstall_hook(FuncHook* hook)
{
	/* Remove from the list. */
	if (hook->prev)
		hook->prev->next = hook->next;
	else
		hook_head = hook->next;
	
	if (hook->next)
		hook->next->prev = hook->prev;
	
	/* Restore original instructions. */
	hook->original_func[0] = hook->original_insns[0];
	hook->original_func[1] = hook->original_insns[1];
	
	/* Flush the instruction cache. */
	FlushCache(2);
}


void uninstall_all_hooks()
{
	for (FuncHook* hook = hook_head; hook != NULL; hook = hook->next)
		uninstall_hook(hook);
}

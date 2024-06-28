#include <racdoor/hook.h>

#include <racdoor/mips.h>

static FuncHook* hook_head;
static CallHook* call_hook_head;

void FlushCache(int mode);

void install_hook(FuncHook* hook, void* original_func, void* replacement_func, void* trampoline)
{
	/* Store information needed to uninstall the hook. */
	hook->prev = NULL;
	hook->next = hook_head;
	hook_head = hook;
	
	hook->original_func = (u32*) original_func;
	hook->trampoline = (u32*) trampoline;
	
	/* Prime the trampoline. */
	hook->trampoline[0] = hook->original_func[0];
	hook->trampoline[1] = MIPS_J(hook->original_func + 2);
	hook->trampoline[2] = hook->original_func[1];
	
	/* Install the hook. */
	hook->original_func[0] = MIPS_J(replacement_func);
	hook->original_func[1] = MIPS_NOP();
	
	/* Flush the instruction cache. */
	FlushCache(2);
}

void uninstall_hook(FuncHook* hook)
{
	/* Remove the hook from the list. */
	if (hook->prev)
		hook->prev->next = hook->next;
	else
		hook_head = hook->next;
	
	if (hook->next)
		hook->next->prev = hook->prev;
	
	/* Restore the original instructions. */
	hook->original_func[0] = hook->trampoline[0];
	hook->original_func[1] = hook->trampoline[2];
	
	/* Flush the instruction cache. */
	FlushCache(2);
}

void install_call_hook(CallHook* hook, void* instruction, void* replacement_func)
{
	/* Store information needed to uninstall the hook. */
	hook->prev = NULL;
	hook->next = call_hook_head;
	call_hook_head = hook;
	
	hook->instruction = instruction;
	hook->original_func = MIPS_GET_TARGET(*hook->instruction);
	
	/* Install the hook. */
	*hook->instruction = MIPS_JAL(replacement_func);
	
	/* Flush the instruction cache. */
	FlushCache(2);
}

void uninstall_call_hook(CallHook* hook)
{
	/* Remove the hook from the list. */
	if (hook->prev)
		hook->prev->next = hook->next;
	else
		call_hook_head = hook->next;
	
	if (hook->next)
		hook->next->prev = hook->prev;
	
	/* Restore the original target. */
	*hook->instruction = MIPS_JAL(hook->original_func);
	
	/* Flush the instruction cache. */
	FlushCache(2);
}

void uninstall_all_hooks()
{
	for (FuncHook* hook = hook_head; hook != NULL; hook = hook->next)
		uninstall_hook(hook);
	
	for (CallHook* hook = call_hook_head; hook != NULL; hook = hook->next)
		uninstall_call_hook(hook);
}

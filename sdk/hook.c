/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <racdoor/hook.h>

#include <racdoor/mips.h>
#include <racdoor/module.h>

/* A function hook that also acts as a trampoline  */
typedef union {
	/* A trampoline template for a hook that hasn't been installed yet. */
	struct {
		/* A manual hook that hasn't been installed yet. */
		u32 trap_1;
		u32 trap_2;
		u32 trap_3;
	} template;
	/* An auto hook that hasn't been installed yet. */
	struct {
		u32 trap;
		u32 jump_original_func;
		u32 jump_replacement_func;
	} auto_template;
	/* A primed trampoline that is ready to be called. */
	struct {
		u32 first_original_insn;
		u32 jump_original_func;
		u32 second_original_insn;
	} trampoline;
} FuncHook;

extern FuncHook _racdoor_autohooks;
extern FuncHook _racdoor_autohooks_end;

void FlushCache(int mode);

void install_hook(void* trampoline, void* original_func, void* replacement_func)
{
	FuncHook* hook = trampoline;
	u32* original_insns = original_func;
	
	/* Prime the trampoline. */
	hook->trampoline.first_original_insn = original_insns[0];
	hook->trampoline.jump_original_func = MIPS_J(&original_insns[2]);
	hook->trampoline.second_original_insn = original_insns[1];
	
	/* Install the hook. */
	original_insns[0] = MIPS_J(replacement_func);
	original_insns[1] = MIPS_NOP();
	
	/* Flush the instruction cache. */
	FlushCache(2);
}

void uninstall_hook(void* trampoline)
{
	FuncHook* hook = trampoline;
	u32* original_func = MIPS_GET_TARGET(hook->trampoline.jump_original_func);
	
	/* Restore the original instructions. */
	original_func[0] = hook->trampoline.first_original_insn;
	original_func[1] = hook->trampoline.second_original_insn;
	
	/* Flush the instruction cache. */
	FlushCache(2);
}

void install_auto_hooks(void)
{
	for (FuncHook* hook = &_racdoor_autohooks; hook < &_racdoor_autohooks_end; hook++)
	{
		void* original_func = MIPS_GET_TARGET(hook->auto_template.jump_original_func);
		void* replacement_func = MIPS_GET_TARGET(hook->auto_template.jump_replacement_func);
		install_hook(hook, original_func, replacement_func);
	}
}

MODULE_LOAD_FUNC(install_auto_hooks);

void uninstall_auto_hooks(void)
{
	for (FuncHook* hook = &_racdoor_autohooks; hook < &_racdoor_autohooks_end; hook++)
		uninstall_hook(hook);
}

MODULE_UNLOAD_FUNC(uninstall_auto_hooks);

void install_call_hook(CallHook* hook, void* call_insn, void* replacement_func)
{
	/* Store information needed to uninstall the hook. */
	hook->call_insn = call_insn;
	hook->original_func = MIPS_GET_TARGET(*hook->call_insn);
	
	/* Install the hook. */
	*hook->call_insn = MIPS_JAL(replacement_func);
	
	/* Flush the instruction cache. */
	FlushCache(2);
}

void uninstall_call_hook(CallHook* hook)
{
	/* Restore the original target. */
	*hook->call_insn = MIPS_JAL(hook->original_func);
	
	/* Flush the instruction cache. */
	FlushCache(2);
}

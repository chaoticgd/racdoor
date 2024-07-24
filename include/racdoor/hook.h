/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#ifndef _RACDOOR_HOOK_H
#define _RACDOOR_HOOK_H

#include <racdoor/util.h>

/* Generate a template for the trampoline of a function hook that is to be
   installed and uninstalled manually. */
#define FUNC_HOOK(trampoline, returntype) \
	returntype trampoline(); \
	__asm__ \
	( \
		".pushsection .text\n" \
		".global " #trampoline "\n" \
		#trampoline ":\n" \
		"# Make sure we crash if we jump into an unprimed trampoline.\n" \
		"teq $zero, $zero\n" \
		"teq $zero, $zero\n" \
		"teq $zero, $zero\n" \
		".popsection\n" \
	);

/* Patch the start of the original function with a jump instruction pointing
   to the replacement function. */
void install_hook(void* trampoline, void* original_func, void* replacement_func);

/* Restore the start of the original function with the instructions that were
   present before it was hooked. */
void uninstall_hook(void* trampoline);

/* Generate a template for the trampoline of a function hook that is to be
   installed and uninstalled automatically. */
#define AUTO_HOOK(original_func, replacement_func, trampoline, returntype) \
	returntype trampoline(); \
	__asm__ \
	( \
		"# These will end up inside .text in an array.\n" \
		".pushsection .racdoor.autohooks\n" \
		"# Backup the current reorder setting.\n" \
		".set push\n" \
		"# Prevent delay slots from being generated for the jumps.\n" \
		".set noreorder\n" \
		".global " #trampoline "\n" \
		#trampoline ":\n" \
		"# Make sure we crash if we jump into an unprimed trampoline.\n" \
		"teq $zero, $zero\n" \
		"# Data to be passed to install_hook. Due to EE hardware errata, we\n" \
		"# encode this data as a pair of jump instructions to avoid bad opcodes.\n" \
		"j " #original_func "\n" \
		"j " #replacement_func "\n" \
		"# Restore the previous section.\n" \
		".popsection\n" \
		"# Restore the previous noreorder setting.\n" \
		".set pop\n" \
	);

/* Install all hooks defined using the AUTO_HOOK macro. */
void install_auto_hooks(void);

/* Uninstall all hooks defined using the AUTO_HOOK macro. */
void uninstall_auto_hooks(void);

/* A function call hook. */
typedef struct _CallHook {
	u32* call_insn;
	void* original_func;
} CallHook;

/* Patch a function call to change its target. */
void install_call_hook(CallHook* hook, void* call_insn, void* replacement_func);

/* Restore the function call to its original target. */
void uninstall_call_hook(CallHook* hook);

#endif

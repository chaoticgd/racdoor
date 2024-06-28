#ifndef _RACDOOR_HOOK_H
#define _RACDOOR_HOOK_H

#include <racdoor/util.h>

typedef struct _FuncHook {
	struct _FuncHook* prev;
	struct _FuncHook* next;
	u32* original_func;
	u32* trampoline;
} FuncHook;

/* Macro for generating trampolines that are to be used to call the original
   hooked functions from within the replacements. */
#define TRAMPOLINE(name, returntype) \
	returntype name(); \
	__asm__ \
	( \
		".text\n" \
		".global " #name "\n" \
		#name ":\n" \
		"break\n" /* First original instruction. */ \
		"break\n" /* Jump to the original function. */ \
		"break\n" /* Second original instruction. */ \
	);

/* Patch the start of the original function with a jump instruction pointing
   to the replacement function. */
void install_hook(FuncHook* hook, void* original_func, void* replacement_func, void* trampoline);

/* Restore the start of the original function with the instructions that were
   present before it was hooked. */
void uninstall_hook(FuncHook* hook);

/* A function call hook. */
typedef struct _CallHook {
	struct _CallHook* prev;
	struct _CallHook* next;
	u32* instruction;
	void* original_func;
} CallHook;

/* Patch a function call to change its target. */
void install_call_hook(CallHook* hook, void* instruction, void* replacement_func);

/* Restore the function call to its original target. */
void uninstall_call_hook(CallHook* hook);

/* Restore all functions that have been hooked. */
void uninstall_all_hooks();

#endif

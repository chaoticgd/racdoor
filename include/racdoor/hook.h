#ifndef _RACDOOR_HOOK_H
#define _RACDOOR_HOOK_H

#include <racdoor/util.h>

#ifdef __cplusplus
extern "C" {
#endif

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
	asm \
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

/* Restore all functions that have been hooked. */
void uninstall_all_hooks();

#ifdef __cplusplus
}
#endif

#endif

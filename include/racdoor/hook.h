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
	u32 original_insns[2];
} FuncHook;

/* Patch the start of the original function with a jump instruction pointing
   to the replacement function. */
void install_hook(FuncHook* hook, void* original_func, void* replacement_func);

/* Restore the start of the original function with the instructions that were
   present before it was hooked. */
void uninstall_hook(FuncHook* hook);

/* Restore all functions that have been hooked. */
void uninstall_all_hooks();

#ifdef __cplusplus
}
#endif

#endif

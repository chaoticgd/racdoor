#include <game/fonts.h>

#include <racdoor/hook.h>
#include <racdoor/module.h>

extern int Level;

void FontPrintWrapper1(int x, int y, u32 rgba, const char* s, int length);
int DrawBoltCount(void* unknown);

FuncHook hook = {};
TRAMPOLINE(hook_trampoline, int);

int MyDrawBoltCount(void* unknown)
{
	FontPrintRight(200, 100, 0xff00ffff, "Hello, World!", 13);
	
	return hook_trampoline(unknown);
}

void hello_load(void)
{
	install_hook(&hook, DrawBoltCount, MyDrawBoltCount, hook_trampoline);
}

MODULE_LOAD_FUNC(hello_load);

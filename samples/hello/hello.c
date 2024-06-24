#include <racdoor/hook.h>
#include <racdoor/module.h>

extern int Level;

void FontPrintWrapper1(int x, int y, u32 rgba, const char* s, int length);
int DrawBoltCount(void* unknown);

FuncHook hook = {};
TRAMPOLINE(hook_trampoline, int);

int MyDrawBoltCount(void* unknown)
{
	FontPrintWrapper1(200, 100, 0xff00ffff, "Hello, World!", 13);
	
	hook_trampoline(unknown);
}

void mod_load()
{
	install_hook(&hook, DrawBoltCount, MyDrawBoltCount, hook_trampoline);
}

void mod_unload()
{
	
}

MODULE(mod_load, mod_unload);

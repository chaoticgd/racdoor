#include <racdoor/cleanup.h>
#include <racdoor/linker.h>
#include <racdoor/hook.h>

extern int Level;

void FontPrint1Wrapper(int x, int y, u32 rgba, const char* s, int length);
int DrawBoltCount(void* unknown);

FuncHook hook = {};
TRAMPOLINE(hook_trampoline, int);

int MyDrawBoltCount(void* unknown)
{
	FontPrint1Wrapper(200, 100, 0xff00ffff, "Hello, World!", 13);
	
	hook_trampoline(unknown);
}

void racdoor_entry()
{
	apply_relocations();
	cleanup();
	
	install_hook(&hook, DrawBoltCount, MyDrawBoltCount, hook_trampoline);
}

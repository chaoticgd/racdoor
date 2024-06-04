#include <game/dl.h>
#include <racdoor/linker.h>

extern "C" int Level;

extern "C" void racdoor_entry()
{
	apply_relocations();
	
	Level = 123;
	MemSlots.os = 0;
}

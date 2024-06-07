#include <game/dl.h>
#include <racdoor/linker.h>
#include <racdoor/hook.h>

extern "C" int Level;

enum FontAlignment {
	TOP_LEFT = 0,
	TOP_CENTER = 1,
	TOP_RIGHT = 2,
	CENTER_LEFT = 3,
	CENTER_CENTER = 4,
	CENTER_RIGHT = 5,
	BOTTOM_LEFT = 6,
	BOTTOM_CENTER = 7,
	BOTTOM_RIGHT = 8
};
typedef long unsigned int uint64;

extern "C" void FontPrint(
	float x,
	float y,
	uint64 rgba,
	char *s,
	int length,
	float scale_x,
	float scale_y, 
	FontAlignment alignment,
	bool enable_drop_shadow,
	uint64 drop_shadow_color,
	float drop_shadow_x_offset,
	float drop_shadow_y_offset);

void my_font_print(
	float x,
	float y,
	uint64 rgba,
	char *s,
	int length,
	float scale_x,
	float scale_y, 
	FontAlignment alignment,
	bool enable_drop_shadow,
	uint64 drop_shadow_color,
	float drop_shadow_x_offset,
	float drop_shadow_y_offset)
{
	
}

FuncHook myhook;

extern "C" void racdoor_entry()
{
	apply_relocations();
	
	Level = 123;
	MemSlots.os = 0;
	
	install_hook(&myhook, (void*) FontPrint, (void*) my_font_print);
}

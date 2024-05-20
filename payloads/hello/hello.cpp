typedef unsigned long uint64;

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

extern "C" void FontPrint(float x, float y, uint64 rgba, char *s, int length, float scale_x, float scale_y, FontAlignment alignment, bool enable_drop_shadow, uint64 drop_shadow_color, float drop_shadow_x_offset, float drop_shadow_y_offset);

extern "C" void racdoor_entry()
{
	FontPrint(
		10.f,
		10.f,
		0xff0000ff,
		(char*) "Hello world!",
		12,
		15.f,
		30.f,
		CENTER_CENTER,
		false,
		0,
		0.f,
		0.f);
}

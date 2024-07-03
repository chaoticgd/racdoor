#include <racdoor/console.h>

#include <game/fonts.h>

#include <racdoor/hook.h>
#include <racdoor/module.h>

#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define LINE_SIZE 128

static char buffer[BUFFER_SIZE];
static u32 buffer_pos = 0;

void DrawHudElements(void);
void draw_hud_elements_thunk(void);
FuncHook draw_hud_elements_hook = {};
TRAMPOLINE(draw_hud_elements_trampoline, int);

void con_load(void)
{
	install_hook(&draw_hud_elements_hook, DrawHudElements, draw_hud_elements_thunk, draw_hud_elements_trampoline);
}

MODULE_LOAD_FUNC(con_load);

void draw_hud_elements_thunk(void)
{
	u32 pos = buffer_pos;
	s32 line_number = 0;
	char line[LINE_SIZE];
	
	u32 i = pos;
	do
	{
		i = (i + BUFFER_SIZE - 1) % BUFFER_SIZE;
		
		if (buffer[i] == '\0' || buffer[i] == '\n')
		{
			char c;
			u32 j = 0;
			do
			{
				c = buffer[(i + 1 + j) % BUFFER_SIZE];
				line[j] = c;
				j++;
			} while (c != '\0' && c != '\n' && j < LINE_SIZE);
			
			FontPrintNormal(25, 425 - line_number * 20, 0xff00ffff, line, j - 1);
			
			line_number++;
		}
	} while (i != pos && buffer[i] != '\0');
	
	draw_hud_elements_trampoline();
}

void con_puts(const char* string)
{
	int pos = buffer_pos;
	
	size_t size = strlen(string);
	for (size_t i = 0; i < size; i++)
	{
		buffer[pos] = string[i];
		pos = (pos + 1) % BUFFER_SIZE;
	}
	
	buffer[pos] = '\0';
	
	buffer_pos = pos;
}

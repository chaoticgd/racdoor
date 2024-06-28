#include <racdoor/console.h>

#include <game/fonts.h>

#include <racdoor/hook.h>
#include <racdoor/module.h>

#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 1024

static char display_buffer[BUFFER_SIZE];

void DrawHudElements(void);
void draw_hud_elements_thunk(void);
FuncHook draw_hud_elements_hook = {};
TRAMPOLINE(draw_hud_elements_trampoline, int);

void con_load(void)
{
	install_hook(&draw_hud_elements_hook, DrawHudElements, draw_hud_elements_thunk, draw_hud_elements_trampoline);
}

void draw_hud_elements_thunk(void)
{
	s32 last_newline = BUFFER_SIZE;
	s32 line = 0;
	
	for (s32 i = BUFFER_SIZE; i >= 0; i--)
	{
		if (i == 0 || display_buffer[i - 1] == '\n')
		{
			if (i < BUFFER_SIZE)
				FontPrintNormal(25, 425 - line * 20, 0xff00ffff, &display_buffer[i], last_newline - i);
			
			last_newline = i - 1;
			line++;
		}
	}
	
	draw_hud_elements_trampoline();
}

void con_puts(const char* string)
{
	int size = strlen(string);
	
	for (int i = 0; i < BUFFER_SIZE - size; i++)
		display_buffer[i] = display_buffer[i + size];
	
	memcpy(display_buffer + BUFFER_SIZE - size, string, size);
}

MODULE_LOAD_FUNC(con_load);

#include <game/bloaders.h>
#include <game/gameglobals.h>

#include <racdoor/hook.h>
#include <racdoor/module.h>

void PauseGame(int screen);
void pause_game_thunk(int screen);
FuncHook pause_game_hook = {};
TRAMPOLINE(pause_game_trampoline, void);

void debugmenu_load()
{
	LoadDebugFont();
	
	install_hook(&pause_game_hook, PauseGame, pause_game_thunk, pause_game_trampoline);
}

void pause_game_thunk(int screen)
{
	pause_game_trampoline(screen);
	
	if (screen == 10)
		gameMode = GAME_MODE_DEBUG;
		LoadDebugFont();
}

MODULE_LOAD_FUNC(debugmenu_load);

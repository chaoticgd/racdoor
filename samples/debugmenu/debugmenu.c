#include <game/bloaders.h>
#include <game/gameglobals.h>

#include <racdoor/hook.h>
#include <racdoor/module.h>

void PauseGame(int screen);
void pause_game_thunk(int screen);
AUTO_HOOK(PauseGame, pause_game_thunk, pause_game_trampoline, void);

void pause_game_thunk(int screen)
{
	pause_game_trampoline(screen);
	
	if (screen == 10)
	{
		gameMode = GAME_MODE_DEBUG;
		
		LoadDebugFont();
	}
}

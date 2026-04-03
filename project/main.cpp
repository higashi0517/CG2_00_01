#include <Windows.h>
#include "Game.h"
#include "D3DResourceLeakChecker.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	CoInitializeEx(0, COINIT_MULTITHREADED);

	D3DResourceLeakChecker leakChecker;

	Framework* game = new Game();
	game->Run();

	delete game;
	return 0;
}
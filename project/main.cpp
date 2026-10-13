#include <Windows.h>
#include <memory>
#include "Game.h"
#include "D3DResourceLeakChecker.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	CoInitializeEx(0, COINIT_MULTITHREADED);

	D3DResourceLeakChecker leakChecker;

	std::unique_ptr<Framework> game = std::make_unique<Game>();
	game->Run();

	return 0;
}
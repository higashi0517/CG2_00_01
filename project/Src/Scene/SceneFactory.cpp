#include "SceneFactory.h"
#include "TitleScene.h"
#include "GamePlayScene.h"
#include "ClearScene.h"
#include "GameOverScene.h"

std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName)
{
	std::unique_ptr<BaseScene> newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = std::make_unique<TitleScene>();
	}
	else if (sceneName == "GAMEPLAY") {
		newScene = std::make_unique<GamePlayScene>();
	}
	else if (sceneName == "CLEAR") {
		newScene = std::make_unique<ClearScene>();
	}
	else if (sceneName == "GAMEOVER") {
		newScene = std::make_unique<GameOverScene>();
	}
	
	return newScene;
}

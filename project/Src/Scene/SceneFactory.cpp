#include "SceneFactory.h"
#include "TitleScene.h"
#include "GamePlayScene.h"
#include "ClearScene.h"
#include "GameOverScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName)
{
	BaseScene* newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = new TitleScene();
	}
	else if (sceneName == "GAMEPLAY") {
		newScene = new GamePlayScene();
	}
	else if (sceneName == "CLEAR") {
		newScene = new ClearScene();
	}
	else if (sceneName == "GAMEOVER") {
		newScene = new GameOverScene();
	}
	
	return newScene;
}

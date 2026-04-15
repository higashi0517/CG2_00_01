#pragma once

#include "AbstractSceneFactory.h"
#include <string>

class BaseScene;
class WinApp;           
class GraphicsDevice;  

class SceneManager
{
private:
	// シングルトン化のためのプライベート化
	SceneManager() = default;
	~SceneManager() = default;
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

public:
	void Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice);
	void ChangeScene(const std::string& sceneName);
	void Update();
	void Draw();
	static SceneManager* GetInstance();
	void Finalize();

	void SetSceneFactory(AbstractSceneFactory* sceneFactory) { sceneFactory_ = sceneFactory; }

private:
	BaseScene* scene_ = nullptr;
	BaseScene* nextScene_ = nullptr;
	WinApp* winApp_ = nullptr;
	GraphicsDevice* graphicsDevice_ = nullptr;

	AbstractSceneFactory* sceneFactory_ = nullptr;
};
#include "SceneManager.h"
#include "BaseScene.h"
#include <cassert>

// インスタンスの実体を取得
SceneManager* SceneManager::GetInstance()
{
	static SceneManager instance;
	return &instance;
}

void SceneManager::Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)
{
	winApp_ = winApp;
	graphicsDevice_ = graphicsDevice;
}

void SceneManager::Update()
{
	if (nextScene_) {
		if (scene_) {
			scene_->Finalize();
			delete scene_;
		}
		scene_ = nextScene_;
		nextScene_ = nullptr;

		scene_->Initialize(winApp_, graphicsDevice_);
	}
	if (scene_) {
		scene_->Update();
	}
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);
	nextScene_ = sceneFactory_->CreateScene(sceneName);
}

void SceneManager::Draw()
{
	if (scene_) {
		scene_->Draw();
	}
}

void SceneManager::Finalize()
{
	if (scene_) {
		scene_->Finalize();
		delete scene_;
		scene_ = nullptr;
	}
}
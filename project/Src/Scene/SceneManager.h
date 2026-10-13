#pragma once

#include "AbstractSceneFactory.h"
#include <string>
#include <memory>
#include <utility>
#include <array>

class BaseScene;
class WinApp;           
class GraphicsDevice;  
class SpriteManager;
class Sprite;

class SceneManager
{
private:
	// シングルトン化のためのプライベート化
	SceneManager() = default;
	~SceneManager();
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

public:
	void Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice);
	void ChangeScene(const std::string& sceneName);
	// 結果データをコンストラクタで渡したシーンも予約できる。
	void ChangeScene(std::unique_ptr<BaseScene> nextScene);
	void Update();
	void Draw();
	static SceneManager* GetInstance();
	void Finalize();
	bool IsTransitioning() const { return nextScene_ != nullptr || transition_ != Transition::None; }

	void SetSceneFactory(std::unique_ptr<AbstractSceneFactory> sceneFactory) { sceneFactory_ = std::move(sceneFactory); }

private:
	enum class Transition { None, FadeOut, FadeIn };
	// 60fpsで片道0.4秒。パネルが画面を覆ってからシーンを入れ替える。
	static constexpr int kFadeFrames = 24;
	Transition transition_ = Transition::None;
	int fadeFrames_ = 0;
	std::unique_ptr<SpriteManager> transitionSpriteManager_;
	static constexpr size_t kPanelCount = 3;
	std::array<std::unique_ptr<Sprite>, kPanelCount> transitionPanels_;
	std::array<std::unique_ptr<Sprite>, kPanelCount> transitionLines_;
	void SwitchScene();
	void DrawTransition();

	std::unique_ptr<BaseScene> scene_ = nullptr;
	std::unique_ptr<BaseScene> nextScene_ = nullptr;
	WinApp* winApp_ = nullptr;
	GraphicsDevice* graphicsDevice_ = nullptr;

	std::unique_ptr<AbstractSceneFactory> sceneFactory_ = nullptr;
};

#pragma once
#include "BaseScene.h"
#include "Input.h"
#include "Camera.h"
#include "RailCamera.h"
#include "Object3DManager.h"
#include "Object3D.h"
#include "Player.h"
#include "Skydome.h"
#include "SpriteManager.h"
#include "GameUi.h"
#include "Skeleton.h"
#include <array>
#include <memory>

// タイトルだけの描画・入力・開始演出を所有する。
class TitleScene : public BaseScene
{
public:
	void Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice) override;
	void Update() override;
	void Draw() override;
	void Finalize() override;

private:
	void UpdateAnimation();
	std::unique_ptr<Input> input_;
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<Object3DManager> object3DManager_;
	std::unique_ptr<SpriteManager> spriteManager_;
	std::unique_ptr<GameUi> gameUi_;
	std::unique_ptr<Player> player_;
	std::unique_ptr<Skydome> skydome_;
	std::unique_ptr<RailCamera> titleRailCamera_;
	std::array<std::unique_ptr<Object3D>, 2> titleEnemies_;
	std::unique_ptr<Sprite> titleSprite_;
	SkinCluster skinCluster_;
	float titleTime_ = 0.0f;
	int titleLaunchFrames_ = 0;
	bool titleLaunching_ = false;
	static constexpr int kTitleLaunchFrames = 48;
};

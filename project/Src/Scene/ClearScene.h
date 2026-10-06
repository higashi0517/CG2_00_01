#pragma once
#include "BaseScene.h"
#include "MissionState.h"
#include "Input.h"
#include "SpriteManager.h"
#include "Sprite.h"
#include "GameUi.h"
#include <memory>

class ClearScene : public BaseScene
{
public:
	explicit ClearScene(const MissionState& result = {}) : result_(result) {}
	void Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice) override;
	void Update() override;
	void Draw() override;
	void Finalize() override;

private:
	// 終了時のスコアをコピーし、ゲームシーンの寿命に依存させない。
	MissionState result_;
	std::unique_ptr<Input> input_;
	std::unique_ptr<SpriteManager> spriteManager_;
	std::unique_ptr<Sprite> screen_;
	std::unique_ptr<GameUi> gameUi_;
};

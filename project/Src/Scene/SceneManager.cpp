#include "SceneManager.h"
#include "BaseScene.h"
#include "GraphicsDevice.h"
#include "SpriteManager.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "WinApp.h"
#include <cassert>
#include <algorithm>
#include <cmath>

SceneManager::~SceneManager() = default;

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
	transition_ = Transition::None;
	fadeFrames_ = 0;
	transitionSpriteManager_ = std::make_unique<SpriteManager>();
	transitionSpriteManager_->Initialize(graphicsDevice);
	const char* texture = "Resources/white1x1.png";
	TextureManager::GetInstance()->LoadTexture(texture);
	for (size_t i = 0; i < kPanelCount; ++i) {
		// 各描画に専用バッファを持たせ、同フレーム内の上書きを防ぐ。
		for (auto* sprite : {&transitionPanels_[i], &transitionLines_[i]}) {
			*sprite = std::make_unique<Sprite>();
			(*sprite)->Initialize(transitionSpriteManager_.get(), texture);
			(*sprite)->SetTextureSize({1.0f, 1.0f});
			(*sprite)->SetAnchorPoint({0.5f, 0.5f});
		}
	}
}

void SceneManager::Update()
{
	if (!scene_ && nextScene_) {
		SwitchScene();
		return;
	}
	if (transition_ == Transition::FadeOut) {
		if (fadeFrames_ < kFadeFrames) {
			++fadeFrames_;
		} else {
			// 全画面をパネルで覆った次の更新で入れ替える。
			SwitchScene();
		}
		return;
	}
	if (transition_ == Transition::FadeIn) {
		if (--fadeFrames_ == 0) {
			transition_ = Transition::None;
		}
		// 演出中はゲーム進行とシーンの入力を停止する。
		return;
	}
	if (scene_) {
		scene_->Update();
	}
}

void SceneManager::SwitchScene()
{
	graphicsDevice_->WaitForGPU();
	if (scene_) {
		scene_->Finalize();
	}
	scene_ = std::move(nextScene_);
	scene_->Initialize(winApp_, graphicsDevice_);
	fadeFrames_ = kFadeFrames;
	transition_ = Transition::FadeIn;
}

void SceneManager::ChangeScene(const std::string& sceneName)
{
	assert(sceneFactory_);
	if (!sceneFactory_ || IsTransitioning()) { return; }
	ChangeScene(sceneFactory_->CreateScene(sceneName));
}

void SceneManager::ChangeScene(std::unique_ptr<BaseScene> nextScene)
{
	assert(nextScene);
	if (!nextScene || IsTransitioning()) { return; }
	nextScene_ = std::move(nextScene);
	if (scene_) {
		transition_ = Transition::FadeOut;
		fadeFrames_ = 0;
	}
}

void SceneManager::Draw()
{
	if (scene_) {
		scene_->Draw();
	}
	if (transitionSpriteManager_ && transition_ != Transition::None) {
		DrawTransition();
	}
}

void SceneManager::DrawTransition()
{
	transitionSpriteManager_->SetCommonRenderState();
	const bool closing = transition_ == Transition::FadeOut;
	const float progress = closing ? float(fadeFrames_) / float(kFadeFrames)
		: 1.0f - float(fadeFrames_) / float(kFadeFrames);
	const float centerX = float(WinApp::kClientWidth) * 0.5f;
	const float centerY = float(WinApp::kClientHeight) * 0.5f;
	constexpr float kWidth = 1900.0f;
	constexpr float kHeight = 1400.0f;
	constexpr float kTilt = -0.18f;
	constexpr Vector4 colors[kPanelCount] = {
		{0.035f, 0.32f, 0.46f, 1.0f},
		{0.02f, 0.12f, 0.24f, 1.0f},
		{0.008f, 0.016f, 0.04f, 1.0f}
	};
	for (size_t i = 0; i < kPanelCount; ++i) {
		// 3枚を少しずつ遅らせ、加速・減速しながら同じ方向へ通過させる。
		const float delay = float(i) * 0.10f;
		const float t = std::clamp((progress - delay) / (1.0f - delay), 0.0f, 1.0f);
		const float eased = t * t * (3.0f - 2.0f * t);
		const float x = closing ? centerX - 2340.0f * (1.0f - eased)
			: centerX + 2340.0f * eased;
		Sprite& panel = *transitionPanels_[i];
		panel.SetSize({kWidth, kHeight});
		panel.SetPosition({x, centerY});
		panel.SetRotation(kTilt);
		panel.SetColor(colors[i]);
		panel.Update();
		panel.Draw();

		// 閉じる時は先端、開く時は後端にシアンの光を走らせる。
		const float edge = (closing ? 1.0f : -1.0f) * kWidth * 0.5f;
		Sprite& line = *transitionLines_[i];
		line.SetSize({i == kPanelCount - 1 ? 5.0f : 11.0f, kHeight});
		line.SetPosition({x + std::cos(kTilt) * edge, centerY + std::sin(kTilt) * edge});
		line.SetRotation(kTilt);
		line.SetColor({0.25f, 0.85f, 1.0f, 0.9f});
		line.Update();
		line.Draw();
	}
}

void SceneManager::Finalize()
{
	nextScene_.reset();
	if (scene_) {
		scene_->Finalize();
		scene_.reset();
	}
	sceneFactory_.reset();
	for (auto& line : transitionLines_) { line.reset(); }
	for (auto& panel : transitionPanels_) { panel.reset(); }
	transitionSpriteManager_.reset();
	transition_ = Transition::None;
	fadeFrames_ = 0;
	winApp_ = nullptr;
	graphicsDevice_ = nullptr;
}

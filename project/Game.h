#pragma once

#include "WinApp.h"
#include "GraphicsDevice.h"
#include "Input.h"
#include "Sound.h"
#include "SrvManager.h"
#include "ImGuiManager.h"
#include "DebugCamera.h"
#include "Object3DManager.h"
#include "Object3D.h"
#include "SpriteManager.h"
#include "ParticleManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "ParticleEmitter.h"
#include "Framework.h"
#include "GamePlayScene.h"

class Game :public Framework {
public:
	Game();
	~Game();

	void Initialize() override;
	void Update() override;
	void Draw() override;
	void Finalize() override;

private:
	// ==== 基盤システム群 ====
	WinApp* winApp_ = nullptr;
	GraphicsDevice* graphicsDevice_ = nullptr;
	Input* input_ = nullptr;
	Sound* sound_ = nullptr;

	// ==== マネージャー群 ====
	SrvManager* srvManager_ = nullptr;
	Object3DManager* object3DManager_ = nullptr;
	SpriteManager* spriteManager_ = nullptr;
	ParticleManager* particleManager_ = nullptr;
	ParticleEmitter* emitter_ = nullptr;

	// ==== ゲームオブジェクト群 ====
	Camera* camera_ = nullptr;
	Object3D* object3D_ = nullptr;
	Object3D* object3D_2_ = nullptr;
	std::vector<Sprite*> sprites_;
	Sprite* sprite_ = nullptr;

	// 音声データなど
	Sound::SoundData bgmData_;
};
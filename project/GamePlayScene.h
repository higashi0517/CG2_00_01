#pragma once
#include "ModelManager.h"
#include "ImGuiManager.h"
#include "TextureManager.h"
#include "Input.h"
#include "Sound.h"
#include "Object3DManager.h"
#include "Object3D.h"
#include "SpriteManager.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "Camera.h"
#include <vector>
#include <cstdint>
#include "BaseScene.h"

class WinApp;
class GraphicsDevice;

class GamePlayScene : public BaseScene
{
public:
	void Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)override;
	void Update()override;
	void Draw()override;
	void Finalize()override;

private:
	Input* input_ = nullptr;
	Sound* sound_ = nullptr;

	Object3DManager* object3DManager_ = nullptr;
	SpriteManager* spriteManager_ = nullptr;
	ParticleManager* particleManager_ = nullptr;
	ParticleEmitter* emitter_ = nullptr;

	Camera* camera_ = nullptr;
	Object3D* object3D_ = nullptr;
	Object3D* object3D_2_ = nullptr;
	std::vector<Sprite*> sprites_;
	Sprite* sprite_ = nullptr;

	int selected_ = 0;

	Sound::SoundData bgmData_;

	WinApp* winApp_ = nullptr;
	GraphicsDevice* graphicsDevice_ = nullptr;
};
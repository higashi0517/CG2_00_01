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

	ParticleManager* particleManager_ = nullptr;
	ParticleEmitter* emitterSmall_ = nullptr;
	ParticleEmitter* emitterLarge_ = nullptr;

	Camera* camera_ = nullptr;

	int selected_ = 0;

	WinApp* winApp_ = nullptr;
	GraphicsDevice* graphicsDevice_ = nullptr;
};
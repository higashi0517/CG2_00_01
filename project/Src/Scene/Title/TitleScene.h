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
#include "Skybox.h"

class WinApp;
class GraphicsDevice;

class TitleScene : public BaseScene
{
public:
	void Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)override;
	void Update()override;
	void Draw()override;
	void Finalize()override;

private:
	
	WinApp* winApp_ = nullptr;
	GraphicsDevice* graphicsDevice_ = nullptr;
};
#pragma once
#include "ModelManager.h"
#include "ImGuiManager.h"
#include "TextureManager.h"
#include "Input.h"
#include "Object3DManager.h"
#include "Object3D.h"
#include "SpriteManager.h"
#include "Camera.h"
#include "RailCamera.h"
#include <vector>
#include <cstdint>
#include "BaseScene.h"
#include "Animation.h"
#include "Skeleton.h"
#include "SkinningCompute.h"
#include "Player.h"
#include "EnemySpawnController.h"
#include "Skydome.h"
#include "Projectile.h"
#include "NormalProjectile.h"
#include "HomingProjectile.h"
#include "Reticle3D.h"
#include "GameUi.h"
#include "MissionState.h"

class WinApp;
class GraphicsDevice;
class RenderTexture;

class GamePlayScene : public BaseScene
{
public:
	GamePlayScene();
	~GamePlayScene() override;

	void Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)override;
	void Update()override;
	void Draw()override;
	void Finalize()override;

private:
	MissionState mission_;
	std::unique_ptr<GameUi> gameUi_;
	bool showDebug_ = false;
	int shotCooldown_ = 0;
	int uiFrames_ = 0;
	int gameFadeFrames_ = 0;
	void StartMission();
	void DrawGameUi();
	Input* input_ = nullptr;

	Object3DManager* object3DManager_ = nullptr;
	SpriteManager* spriteManager_ = nullptr;
	std::unique_ptr<Player> player_;
	std::unique_ptr<EnemySpawnController> enemySpawnController_;

	Camera* camera_ = nullptr;
	std::unique_ptr<RailCamera> railCamera_;
	Vector2 railPlayerOffset_ = { 0.0f, 0.0f };
	std::unique_ptr<Skydome> skydome_;
	std::vector<std::unique_ptr<Projectile>> playerBullets_;
	std::vector<std::unique_ptr<Projectile>> enemyBullets_;
	std::unique_ptr<Reticle3D> reticle3D_;
	Enemy* lockOnTarget_ = nullptr;
	int enemyBulletTimer_ = 0;
	/*Object3D* object3D_ = nullptr;
	Object3D* object3D_2_ = nullptr;*/
	std::vector<Sprite*> sprites_;
	Sprite* sprite_ = nullptr;

	int selected_ = 0;
	bool isPlayerEnemyColliding_ = false;


	WinApp* winApp_ = nullptr;
	GraphicsDevice* graphicsDevice_ = nullptr;

	std::unique_ptr<RenderTexture> renderTexture_;
	Sprite* postProcessSprite_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> copyRootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> copyPipelineState_;

	Animation animation_;
	SkinCluster skinCluster_;
	Skeleton skeleton_;
	float animationTime_ = 0.0f;

	std::unique_ptr<SkinningCompute> skinningCompute_;
	uint32_t skinningVertexCount_ = 0;

	void UpdateFollowCamera();
	void SpawnPlayerBullet();
	void SpawnEnemyBullet();
	void UpdateProjectiles();
	void CheckProjectileCollisions();
};

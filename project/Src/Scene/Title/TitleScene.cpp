#include "TitleScene.h"
#include "SceneManager.h"
#include "ModelManager.h"
#include "SrvManager.h"
#include <algorithm>
#include <cmath>

void TitleScene::Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)
{
	input_ = std::make_unique<Input>();
	input_->Initialize(winApp);
	input_->Update();
	camera_ = std::make_unique<Camera>();
	camera_->SetFovY(0.75f);
	camera_->SetFarClip(1000.0f);
	object3DManager_ = std::make_unique<Object3DManager>();
	object3DManager_->Initialize(graphicsDevice);
	object3DManager_->SetDefaultCamera(camera_.get());
	ModelManager::GetInstance()->LoadModel("player/player.obj");
	ModelManager::GetInstance()->LoadModel("enemy/enemy.obj");
	player_ = std::make_unique<Player>();
	player_->Initialize(object3DManager_.get(), "player/player.obj");
	player_->SetScale({0.5f, 0.5f, 0.5f});
	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(graphicsDevice, camera_.get());
	// タイトル専用の閉じたレール。長時間待機しても終端で止まらない。
	titleRailCamera_ = std::make_unique<RailCamera>();
	titleRailCamera_->Initialize(camera_.get(), {
		{0.0f, 0.0f, 0.0f}, {17.6f, 0.5f, 42.4f},
		{60.0f, 1.0f, 60.0f}, {102.4f, 0.5f, 42.4f},
		{120.0f, 0.0f, 0.0f}, {102.4f, -0.5f, -42.4f},
		{60.0f, -1.0f, -60.0f}, {17.6f, -0.5f, -42.4f}
	});
	titleRailCamera_->SetLoop(true);
	titleRailCamera_->SetActive(true);
	for (auto& enemy : titleEnemies_) {
		enemy = std::make_unique<Object3D>();
		enemy->Initialize(object3DManager_.get());
		enemy->SetModel("enemy/enemy.obj");
		enemy->SetScale({0.35f, 0.35f, 0.35f});
	}

	spriteManager_ = std::make_unique<SpriteManager>();
	spriteManager_->Initialize(graphicsDevice);
	gameUi_ = std::make_unique<GameUi>();
	gameUi_->Initialize(spriteManager_.get());
	const char* path = "Resources/UI/title.png";
	TextureManager::GetInstance()->LoadTexture(path);
	const auto& metadata = TextureManager::GetInstance()->GetMetaData(path);
	titleSprite_ = std::make_unique<Sprite>();
	titleSprite_->Initialize(spriteManager_.get(), path);
	// 背景付き画像のロゴ領域だけを使用する。
	titleSprite_->SetTextureLeftTop({float(metadata.width) * 0.275f, float(metadata.height) * 0.44f});
	titleSprite_->SetTextureSize({float(metadata.width) * 0.45f, float(metadata.height) * 0.11f});
	titleSprite_->SetAnchorPoint({0.5f, 0.5f});
	titleSprite_->SetPosition({640.0f, 156.0f});
	titleTime_ = 0.0f;
	titleLaunchFrames_ = 0;
	titleLaunching_ = false;
	UpdateAnimation();
}

void TitleScene::Update()
{
	input_->Update();
	if (!titleLaunching_ && (input_->TriggerKey(DIK_SPACE) || input_->TriggerKey(DIK_RETURN))) {
		titleLaunching_ = true;
	}
	if (!titleLaunching_ && input_->TriggerKey(DIK_ESCAPE)) { PostQuitMessage(0); }
	UpdateAnimation();
}

void TitleScene::Draw()
{
	skydome_->Draw();
	object3DManager_->SetCommonRenderState();
	player_->Draw(skinCluster_);
	for (size_t i = 0; i < titleEnemies_.size(); ++i) {
		if (std::fmod(titleTime_ + float(i) * 4.0f, 8.0f) < 6.0f) {
			titleEnemies_[i]->Draw(skinCluster_);
		}
	}
	spriteManager_->SetCommonRenderState();
	gameUi_->Begin();
	const float launch = float(titleLaunchFrames_) / float(kTitleLaunchFrames);
	const float alpha = 1.0f - launch;
	gameUi_->Rect(0, 0, 1280, 720, {0.01f, 0.025f, 0.07f, 0.22f * alpha});
	titleSprite_->Draw();
	const float blink = titleLaunching_ ? alpha : 0.5f + std::sin(titleTime_ * 3.0f) * 0.5f;
	gameUi_->Rect(360, 590, 560, 60, {0.025f, 0.045f, 0.08f, 0.8f * alpha});
	gameUi_->CenteredText(titleLaunching_ ? "LAUNCHING" : "PRESS SPACE TO START", 606, 1.8f, {0.92f, 0.96f, 1.0f, blink});
	gameUi_->CenteredText("WASD MOVE   MOUSE AIM   HOLD SPACE FIRE", 674, 1.2f, {0.92f, 0.96f, 1.0f, alpha});
	// 最後の0.25秒で暗転し、レール始点への切り替えを隠す。
	const float black = std::clamp((launch - 0.6875f) / 0.3125f, 0.0f, 1.0f);
	gameUi_->Rect(0, 0, 1280, 720, {0.0f, 0.0f, 0.0f, black});
}

void TitleScene::UpdateAnimation()
{
	constexpr float kDeltaTime = 1.0f / 60.0f;
	titleTime_ += kDeltaTime;
	if (titleLaunching_) {
		// 48フレーム目を真っ黒で描画してからゲームへ切り替える。
		if (titleLaunchFrames_ >= kTitleLaunchFrames) {
			SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
			return;
		}
		++titleLaunchFrames_;
	}
	const float launch = float(titleLaunchFrames_) / float(kTitleLaunchFrames);
	const float acceleration = launch * launch;
	titleRailCamera_->SetSpeed((1.0f + 8.0f * acceleration) / (120.0f * 60.0f));
	titleRailCamera_->Update();
	const Vector3 origin = titleRailCamera_->GetRailPosition();
	const Vector3 forward = titleRailCamera_->GetForward();
	const Vector3 right = titleRailCamera_->GetRight();
	const float distance = 16.0f * acceleration;
	player_->SetTranslate({
		origin.x + forward.x * distance,
		origin.y + forward.y * distance + std::sin(titleTime_ * 2.0f) * 0.15f * (1.0f - launch),
		origin.z + forward.z * distance
	});
	Vector3 rotation = titleRailCamera_->GetRailRotation();
	rotation.z = std::sin(titleTime_ * 1.4f) * 0.06f * (1.0f - launch);
	player_->SetRotate(rotation);
	player_->UpdateTransform();

	// 衝突や発砲をしない演出用の敵。奥から左右を横切る。
	for (size_t i = 0; i < titleEnemies_.size(); ++i) {
		const float cycle = std::fmod(titleTime_ + float(i) * 4.0f, 8.0f);
		const float direction = i == 0 ? 1.0f : -1.0f;
		const float side = (-42.0f + cycle * 14.0f) * direction;
		const float depth = 42.0f - cycle * 3.0f;
		const Vector3 position{
			origin.x + forward.x * depth + right.x * side,
			origin.y + forward.y * depth + 3.0f + float(i) * 2.0f,
			origin.z + forward.z * depth + right.z * side
		};
		Vector3 enemyRotation = titleRailCamera_->GetRailRotation();
		enemyRotation.y += direction * 1.5707963f;
		if (cycle > 2.5f && cycle < 3.5f) {
			const Vector3 playerPosition = player_->GetTranslate();
			const float facing = std::atan2(playerPosition.x - position.x, playerPosition.z - position.z);
			const float turn = std::sin((cycle - 2.5f) * 3.1415927f);
			const float difference = std::remainder(facing - enemyRotation.y, 6.2831853f);
			enemyRotation.y += difference * turn;
		}
		titleEnemies_[i]->SetTranslate(position);
		titleEnemies_[i]->SetRotate(enemyRotation);
		titleEnemies_[i]->Update();
	}
	skydome_->Update();
	const float pulse = 1.0f + std::sin(titleTime_ * 2.0f) * 0.03f;
	titleSprite_->SetSize({760.0f * pulse, 104.0f * pulse});
	titleSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f - launch});
	titleSprite_->Update();
}

void TitleScene::Finalize()
{
	titleSprite_.reset();
	gameUi_.reset();
	for (auto& enemy : titleEnemies_) { enemy.reset(); }
	player_.reset();
	skydome_.reset();
	titleRailCamera_.reset();
	object3DManager_.reset();
	spriteManager_.reset();
	camera_.reset();
	input_.reset();
}

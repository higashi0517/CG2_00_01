#include "Game.h"

Game::Game() {}
Game::~Game() {}

void Game::Initialize() {

	// 基盤システムの初期化
	Framework::Initialize();

	winApp_ = new WinApp();
	winApp_->Initialize();

	graphicsDevice_ = new GraphicsDevice();
	graphicsDevice_->Initialize(winApp_);

	srvManager_ = SrvManager::GetInstance();
	srvManager_->Initialize(graphicsDevice_);

	TextureManager::GetInstance()->Initialize(graphicsDevice_, srvManager_);

	scene_ = new GamePlayScene();
	scene_->Initialize(winApp_, graphicsDevice_);
}

void Game::Update() {

	// 基盤システムの更新
	Framework::Update();
}

void Game::Draw() {

	// 描画前処理
	graphicsDevice_->PreDraw();
	srvManager_->PreDraw();

	Framework::Draw();

	// === ImGui描画 ===
	ImGuiManager::GetInstance()->Draw();

	// 描画後処理
	graphicsDevice_->PostDraw();
}

void Game::Finalize() {

	// === ここから下は基盤やマネージャーなので残す ===
	TextureManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();

	winApp_->Finalize();
	ImGuiManager::GetInstance()->Finalize();

	delete winApp_;
	delete graphicsDevice_;

	Framework::Finalize();
}
#include "Game.h"
#include"SceneFactory.h"

Game::Game() {}
Game::~Game() {}

void Game::Initialize() {

	// 基盤システムの初期化
	Framework::Initialize();

	winApp_ = std::make_unique<WinApp>();
	winApp_->Initialize();

	graphicsDevice_ = std::make_unique<GraphicsDevice>();
	graphicsDevice_->Initialize(winApp_.get());

	srvManager_ = SrvManager::GetInstance();
	srvManager_->Initialize(graphicsDevice_.get());

	ImGuiManager::GetInstance()->Initialize(winApp_.get(), graphicsDevice_.get());

	TextureManager::GetInstance()->Initialize(graphicsDevice_.get(), srvManager_);
	// モデルはシーンをまたいで共有するため、アプリ起動時に一度だけ初期化する。
	ModelManager::GetInstance()->Initialize(graphicsDevice_.get());

	SceneManager::GetInstance()->Initialize(winApp_.get(), graphicsDevice_.get());
	SceneManager::GetInstance()->SetSceneFactory(std::make_unique<SceneFactory>());
	SceneManager::GetInstance()->ChangeScene("TITLE");
}

void Game::Update() {

	ImGuiManager::GetInstance()->Begin();

	// 基盤システムの更新
	Framework::Update();
}

void Game::Draw() {

	// 描画前処理
	graphicsDevice_->PreDraw();
	srvManager_->PreDraw();

	Framework::Draw();

	ImGuiManager::GetInstance()->End();

	// === ImGui描画 ===
	ImGuiManager::GetInstance()->Draw();

	// 描画後処理
	graphicsDevice_->PostDraw();
}

void Game::Finalize() {
	// シーンが所有する描画リソースを、基盤より先に解放する。
	graphicsDevice_->WaitForGPU();
	SceneManager::GetInstance()->Finalize();
	ImGuiManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
	Framework::Finalize();
	winApp_->Finalize();
	winApp_.reset();
	graphicsDevice_.reset();
}
bool Game::ProcessMessage() {
	// WinAppのメッセージループを回す
	return winApp_->ProcessMessage();
}

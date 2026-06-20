#include "TitleScene.h"
#include "SceneManager.h"
#include "GamePlayScene.h"
#include "SrvManager.h"

void TitleScene::Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)
{
	winApp_ = winApp;
	graphicsDevice_ = graphicsDevice;

	// 3Dモデルマネジャの初期化
	ModelManager::GetInstance()->Initialize(graphicsDevice_);
}

void TitleScene::Update() {

}

void TitleScene::Draw() {

}

void TitleScene::Finalize() {

}
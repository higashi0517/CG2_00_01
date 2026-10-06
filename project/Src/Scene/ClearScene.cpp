#include "ClearScene.h"
#include "SceneManager.h"
#include "TextureManager.h"
#include <string>

void ClearScene::Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)
{
	input_ = std::make_unique<Input>();
	input_->Initialize(winApp);
	// シーンを切り替えた瞬間の押しっぱなしを、新しい決定入力にしない。
	input_->Update();
	spriteManager_ = std::make_unique<SpriteManager>();
	spriteManager_->Initialize(graphicsDevice);
	gameUi_ = std::make_unique<GameUi>();
	gameUi_->Initialize(spriteManager_.get());
	const char* path = "Resources/UI/clear.png";
	TextureManager::GetInstance()->LoadTexture(path);
	const auto& metadata = TextureManager::GetInstance()->GetMetaData(path);
	screen_ = std::make_unique<Sprite>();
	screen_->Initialize(spriteManager_.get(), path);
	screen_->SetTextureSize({float(metadata.width), float(metadata.height)});
	screen_->SetSize({1280.0f, 720.0f});
	screen_->SetPosition({0.0f, 0.0f});
	screen_->Update();
}

void ClearScene::Update()
{
	input_->Update();
	if (input_->TriggerKey(DIK_RETURN)) {
		SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
	}
	else if (input_->TriggerKey(DIK_ESCAPE)) {
		SceneManager::GetInstance()->ChangeScene("TITLE");
	}
}

void ClearScene::Draw()
{
	spriteManager_->SetCommonRenderState();
	screen_->Draw();
	gameUi_->Begin();
	const Vector4 white{0.92f, 0.96f, 1.0f, 1.0f};
	gameUi_->CenteredText("TARGETS " + std::to_string(result_.GetDefeated()) +
		" / 5    HP " + std::to_string(result_.GetHealth()) + " / 3    TIME " +
		std::to_string(result_.GetRemainingSeconds()) + "s", 652, 1.3f, white);
}

void ClearScene::Finalize()
{
	gameUi_.reset();
	screen_.reset();
	spriteManager_.reset();
	input_.reset();
}

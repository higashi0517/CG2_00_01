#include "GamePlayScene.h"

void GamePlayScene::Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)
{
	winApp_ = winApp;
	graphicsDevice_ = graphicsDevice;

	// 3Dモデルマネジャの初期化
	ModelManager::GetInstance()->Initialize(graphicsDevice_);

	input_ = new Input();
	input_->Initialize(winApp_);

	camera_ = new Camera();
	camera_->SetTranslate({ 0.0f, 0.0f, -10.0f });
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });

	particleManager_ = new ParticleManager();
	particleManager_->Initialize(graphicsDevice_);
	particleManager_->SetCamera(camera_);

	particleManager_->CreateParticleGroup("HitEffect", "Resources/circle2.png");

	// 1. 小さいトゲ用エミッターの初期化
	emitterSmall_ = new ParticleEmitter();
	emitterSmall_->Initialize(particleManager_, "HitEffect");
	emitterSmall_->SetEmitCount(15);          
	emitterSmall_->SetScaleYRange(0.5f, 1.2f); 
	emitterSmall_->SetIsEmitting(false);      

	// 2. 大きいトゲ用エミッターの初期化
	emitterLarge_ = new ParticleEmitter();
	emitterLarge_->Initialize(particleManager_, "HitEffect");
	emitterLarge_->SetEmitCount(10);        
	emitterLarge_->SetScaleYRange(2.0f, 3.5f); 
	emitterLarge_->SetIsEmitting(false);    
}

void GamePlayScene::Update() {

	input_->Update();
	if (input_->TriggerKey(DIK_0)) {
		OutputDebugStringA("Hit 0\n");
	}

	// ゲーム処理

#ifdef USE_IMGUI

	// camera
	Vector3 cameraPos = camera_->GetTranslate();

	if (ImGui::DragFloat3("Camera Position", &cameraPos.x, 0.01f)) {
		camera_->SetTranslate(cameraPos);
	}

	// デモウィンドウの表示
	ImGui::ShowDemoWindow();


#endif

	if (input_->TriggerKey(DIK_1)) {
		OutputDebugStringA("Hit 0: Emit HitEffect\n");

		// エフェクトを発生させたい座標（今回は原点にしていますが、本来は敵の座標など）
		Vector3 hitPosition = { 0.0f, 0.0f, 0.0f };

		// 小さいエミッターから1回だけドバッと出す
		emitterSmall_->SetPosition(hitPosition);
		emitterSmall_->Emit();

		// 大きいエミッターから1回だけドバッと出す
		emitterLarge_->SetPosition(hitPosition);
		emitterLarge_->Emit();
	}

	// カメラの更新
	camera_->Update();

	if (emitterSmall_) emitterSmall_->Update();
	if (emitterLarge_) emitterLarge_->Update();

	particleManager_->Update();
}

void GamePlayScene::Draw() {

	// === パーティクル描画 ===
	particleManager_->SetCommonRenderState();
	particleManager_->Draw();

}

void GamePlayScene::Finalize() {

	delete input_;
	delete camera_;

	if (emitterSmall_) delete emitterSmall_;
	if (emitterLarge_) delete emitterLarge_;
	delete particleManager_;
}

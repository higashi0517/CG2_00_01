#include "GamePlayScene.h"

void GamePlayScene::Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)
{
	winApp_ = winApp;
	graphicsDevice_ = graphicsDevice;

	// 3Dモデルマネジャの初期化
	ModelManager::GetInstance()->Initialize(graphicsDevice_);
	// .objモデルの読み込み
	ModelManager::GetInstance()->LoadModel("plane.obj");

	input_ = new Input();
	input_->Initialize(winApp_);

	sound_ = new Sound();

	camera_ = new Camera();
	camera_->SetTranslate({ 0.0f, 0.0f, -10.0f });
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });

	object3DManager_ = new Object3DManager();
	object3DManager_->Initialize(graphicsDevice_);
	object3DManager_->SetDefaultCamera(camera_);

	object3D_ = new Object3D();
	object3D_->Initialize(object3DManager_);
	object3D_->SetModel("plane.obj");

	object3D_2_ = new Object3D();
	object3D_2_->Initialize(object3DManager_);
	object3D_2_->SetModel("plane.obj");

	spriteManager_ = nullptr;
	// スプライト共通部の初期化
	spriteManager_ = new SpriteManager();
	spriteManager_->Initialize(graphicsDevice_);

	TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("Resources/monsterBall.png");

	for (uint32_t i = 0; i < 5; ++i) {
		sprite_ = new Sprite();
		sprite_->Initialize(spriteManager_, "Resources/uvChecker.png");
		if (i % 2 == 1)
		{
			sprite_->ChangeTexture("Resources/monsterBall.png");
		}
		sprite_->SetPosition({ 100.0f + i * 120.0f, 50.0f });
		sprite_->SetSize({ 100.0f,100.0f });
		sprites_.push_back(sprite_);
	}

	// 音声の読み込み
	bgmData_ = sound_->LoadFile("Resources/mokugyo.wav");

	particleManager_ = new ParticleManager();
	particleManager_->Initialize(graphicsDevice_);
	particleManager_->SetCamera(camera_);

	particleManager_->CreateParticleGroup("Magic", "Resources/circle.png");

	emitter_ = new ParticleEmitter();
	emitter_->Initialize(particleManager_, "Magic");
}

void GamePlayScene::Update() {

	input_->Update();
	if (input_->TriggerKey(DIK_0)) {
		OutputDebugStringA("Hit 0\n");
	}

	// soundの再生
	if (input_->TriggerKey(DIK_1)) {
		sound_->PlayWave(bgmData_);
	}


	// ゲーム処理

#ifdef USE_IMGUI

		// Imguiのフレーム開始
	ImGuiManager::GetInstance()->Begin();

	//ImGui::Begin("Sprites");

	ImGui::SliderInt("Selected", &selected_, 0, (int)sprites_.size() - 1);

	Sprite* s = sprites_[selected_];

	// sprite
	Vector2 pos = s->GetPosition();
	float   rot = s->GetRotation();
	Vector2 size = s->GetSize();
	Vector4 col = s->GetColor();

	if (ImGui::DragFloat2("Position", &pos.x, 1.0f)) s->SetPosition(pos);
	if (ImGui::DragFloat("Rotation", &rot, 0.01f))   s->SetRotation(rot);
	if (ImGui::DragFloat2("Size", &size.x, 1.0f))    s->SetSize(size);
	if (ImGui::ColorEdit4("Color", &col.x))          s->SetColor(col);

	// 3d object
	Vector3 pos3D = object3D_->GetTranslate();
	Vector3 rotate3D = object3D_->GetRotate();

	if (ImGui::DragFloat3("3D Position", &pos3D.x, 0.01f)) {
		object3D_->SetTranslate(pos3D);
	}
	if (ImGui::DragFloat3("3D Rotation", &rotate3D.x, 0.01f)) {
		object3D_->SetRotate(rotate3D);
	}

	// camera
	Vector3 cameraPos = camera_->GetTranslate();

	if (ImGui::DragFloat3("Camera Position", &cameraPos.x, 0.01f)) {
		camera_->SetTranslate(cameraPos);
	}

	// デモウィンドウの表示
	ImGui::ShowDemoWindow();

	ImGuiManager::GetInstance()->End();

#endif

	// カメラの更新
	camera_->Update();

	emitter_->Update();

	object3D_->Update();
	//object3D_2_->Update();

	for (auto& sprite : sprites_) {
		sprite->Update();
	}

	particleManager_->Update();
}

void GamePlayScene::Draw() {

	// === 3Dオブジェクト描画 ===
	object3DManager_->SetCommonRenderState();
	//object3D_->Draw();
	// object3D_2_->Draw();

	// === スプライト描画 ===
	spriteManager_->SetCommonRenderState();
	for (auto& sprite : sprites_) {
		sprite->Draw();
	}

	// === パーティクル描画 ===
	particleManager_->SetCommonRenderState();
	particleManager_->Draw();

}

void GamePlayScene::Finalize() {
	sound_->Unload(&bgmData_);

	delete input_;
	delete sound_;
	delete camera_;
	delete object3D_;
	delete object3D_2_;
	delete object3DManager_;

	for (Sprite* sprite : sprites_) {
		delete sprite;
	}
	sprites_.clear();

	delete spriteManager_;
	delete emitter_;
	delete particleManager_;
}

#include "GamePlayScene.h"
#include "SrvManager.h"
#include "RenderTexture.h"
#include <algorithm>
#include <cassert>
#include <cmath>

void GamePlayScene::Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)
{
	winApp_ = winApp;
	graphicsDevice_ = graphicsDevice;

	// 3Dモデルマネジャの初期化
	ModelManager::GetInstance()->Initialize(graphicsDevice_);
	ModelManager::GetInstance()->LoadModel("player/player.obj");
	ModelManager::GetInstance()->LoadModel("enemy/enemy.obj");

	input_ = new Input();
	input_->Initialize(winApp_);

	sound_ = new Sound();

	camera_ = new Camera();
	camera_->SetTranslate({ 0.0f, 3.0f, -7.0f });
	camera_->SetRotate({ 0.35f, 0.0f, 0.0f });
	camera_->SetFovY(0.75f);
	camera_->SetFarClip(1000.0f);
	camera_->Update();

	// Catmull-Rom補間による三次曲線レールカメラ。
	// ゲーム開始時からレールに沿って自動で進む。
	railCamera_ = std::make_unique<RailCamera>();
	// 約180のコースを75秒で進む。緩やかなカーブで前方を見通しやすくする。
	constexpr float kRailSpeed = 1.0f / (75.0f * 60.0f);
	railCamera_->Initialize(camera_, {
		{ 0.0f, 0.0f, -30.0f },
		{ 0.0f, 0.0f,   0.0f },
		{ 2.0f, 0.5f,  30.0f },
		{ 5.0f, 1.0f,  60.0f },
		{ 0.0f, 1.5f,  90.0f },
		{-5.0f, 1.0f, 120.0f },
		{-2.0f, 0.5f, 150.0f },
		{ 0.0f, 0.0f, 180.0f },
		{ 0.0f, 0.0f, 210.0f }
	});
	railCamera_->SetSpeed(kRailSpeed);
	railCamera_->SetLoop(false);
	railCamera_->SetActive(true);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = graphicsDevice_->AllocateRtvHandle();

	// 2. SRVハンドルの確保 (既存のSrvManagerから空き枠を確保)
	uint32_t srvIndex = SrvManager::GetInstance()->Allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE srvCPU = SrvManager::GetInstance()->GetCPUDescriptorHandle(srvIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE srvGPU = SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);

	// 3. RenderTexture のインスタンス化と初期化 (スライドの仕様：1280x720, ClearColorは赤)
	Vector4 clearColor{ 1.0f, 0.0f, 0.0f, 1.0f };
	renderTexture_ = std::make_unique<RenderTexture>();
	renderTexture_->Initialize(
		graphicsDevice_->GetDevice(),
		1280, 720,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		clearColor,
		rtvHandle, srvCPU, srvGPU
	);

	object3DManager_ = new Object3DManager();
	object3DManager_->Initialize(graphicsDevice_);
	object3DManager_->SetDefaultCamera(camera_);

	skydome_ = std::make_unique<Skydome>();
	skydome_->Initialize(graphicsDevice_, camera_);

	player_ = std::make_unique<Player>();
	player_->Initialize(object3DManager_, "player/player.obj");
	player_->SetScale({ 0.5f, 0.5f, 0.5f });
	player_->SetTranslate({ -1.0f, 0.0f, 0.0f });

	enemySpawnController_ = std::make_unique<EnemySpawnController>();
	enemySpawnController_->Initialize(object3DManager_, "enemy/enemy.obj");
	enemySpawnController_->SetDespawnDistance(1000.0f);
	// 15秒おきに、出現時のレール位置から18ほど前方へ配置する。
	// 固定のワールド座標ではなく、コース変更に追従する配置。
	for (int i = 0; i < MissionState::kTargetCount; ++i) {
		const int frame = i * 15 * 60;
		const float progress = float(frame) * kRailSpeed;
		const Vector3 railPosition = railCamera_->GetPositionAtProgress(progress);
		const Vector3 ahead = railCamera_->GetPositionAtProgress(progress + 0.01f);
		const Vector3 forward = Normalize(Vector3{ ahead.x - railPosition.x, ahead.y - railPosition.y, ahead.z - railPosition.z });
		const Vector3 right = Normalize(Vector3{ forward.z, 0.0f, -forward.x });
		const float side = i % 2 == 0 ? -2.5f : 2.5f;
		enemySpawnController_->AddSpawnEvent(frame, {
			railPosition.x + forward.x * 18.0f + right.x * side,
			railPosition.y + forward.y * 18.0f,
			railPosition.z + forward.z * 18.0f + right.z * side
		});
	}
	spriteManager_ = nullptr;
	// スプライト共通部の初期化
	spriteManager_ = new SpriteManager();
	spriteManager_->Initialize(graphicsDevice_);

	TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("Resources/monsterBall.png");
	TextureManager::GetInstance()->LoadTexture("Resources/circle2.png");
	TextureManager::GetInstance()->LoadTexture("Resources/white1x1.png");

	reticle3D_ = std::make_unique<Reticle3D>();
	reticle3D_->Initialize(
		spriteManager_,
		winApp_,
		"Resources/circle2.png");
	gameUi_ = std::make_unique<GameUi>();
	gameUi_->Initialize(spriteManager_);
	const std::array<const char*, 3> screenPaths = {
		"Resources/UI/title.png", "Resources/UI/clear.png", "Resources/UI/game-over.png"
	};
	for (size_t i = 0; i < screenImages_.size(); ++i) {
		TextureManager::GetInstance()->LoadTexture(screenPaths[i]);
		const auto& metadata = TextureManager::GetInstance()->GetMetaData(screenPaths[i]);
		auto& screen = screenImages_[i];
		screen = std::make_unique<Sprite>();
		screen->Initialize(spriteManager_, screenPaths[i]);
		screen->SetTextureSize({ float(metadata.width), float(metadata.height) });
		screen->SetSize({ 1280.0f, 720.0f });
		screen->SetPosition({ 0.0f, 0.0f });
		screen->Update();
	}

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

	particleManager_->CreateParticleGroup("Magic", "Resources/gradationLine.png");

	emitter_ = new ParticleEmitter();
	emitter_->Initialize(particleManager_, "Magic");
	emitter_->SetEmitCount(1);
	emitter_->SetScaleYRange(1.0f, 1.0f);

	// --- 1. RootSignature の作成 ---
	D3D12_DESCRIPTOR_RANGE srvRange{};
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = 1;
	srvRange.BaseShaderRegister = 0; // t0
	srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[1]{};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &srvRange;

	// サンプラーの設定 (s0)
	D3D12_STATIC_SAMPLER_DESC staticSampler{};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.ShaderRegister = 0; // s0
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &staticSampler;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// ★修正: RootSignature のシリアライズと作成を有効化
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) {
			OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
		}
		assert(false);
	}
	hr = graphicsDevice_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&copyRootSignature_));
	assert(SUCCEEDED(hr));


	// --- 2. PipelineState (PSO) の作成 ---
	auto vertexShaderBlob = graphicsDevice_->CompileShader(L"Resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
	auto pixelShaderBlob = graphicsDevice_->CompileShader(L"Resources/shaders/Vignette.PS.hlsl", L"ps_6_0");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = copyRootSignature_.Get();
	psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

	// 頂点バッファを使わないため、InputLayout は空にする
	psoDesc.InputLayout = { nullptr, 0 };

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// 共通の設定（不透明描画または通常のブレンド）
	psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// カリングは行わない（三角形が画面より大きいため）
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

	// 深度テストは行わない（ただの画面コピーのため）
	psoDesc.DepthStencilState.DepthEnable = FALSE;

	// 出力先（バックバッファ）のフォーマット（例: R8G8B8A8_UNORM）
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	// サンプルマスクを適切に設定する (0 のままだと何も描画されません)
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // もしくは 0xFFFFFFFF

	// レンダーターゲットの設定
	psoDesc.NumRenderTargets = 1;

	// フォーマットをバックバッファに合わせて _SRGB に変更する
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// コメントアウトを解除し、実際にPipelineStateオブジェクトを生成
	hr = graphicsDevice_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&copyPipelineState_));
	assert(SUCCEEDED(hr));
}

void GamePlayScene::Update() {

	input_->Update();
	++uiFrames_;
	if (input_->TriggerKey(DIK_F1)) { showDebug_ = !showDebug_; }
	if (mission_.GetPhase() != MissionState::Phase::Playing) {
		if (input_->TriggerKey(DIK_RETURN)) {
			StartMission();
		}
		else if (input_->TriggerKey(DIK_ESCAPE)) {
			if (mission_.GetPhase() == MissionState::Phase::Title) { PostQuitMessage(0); }
			else { mission_.ReturnToTitle(); }
		}
		return;
	}
	if (input_->TriggerKey(DIK_ESCAPE)) {
		mission_.ReturnToTitle();
		return;
	}
	mission_.Tick();
	if (shotCooldown_ > 0) { --shotCooldown_; }
	if (showDebug_ && input_->TriggerKey(DIK_R)) {
		railCamera_->ToggleActive();
	}
	if (input_->TriggerKey(DIK_0)) {
		OutputDebugStringA("Hit 0\n");
	}

	// soundの再生
	if (input_->TriggerKey(DIK_1)) {
		sound_->PlayWave(bgmData_);
	}

	// ゲーム処理

#ifdef USE_IMGUI
	if (showDebug_) {

		// Imguiのフレーム開始
	//ImGuiManager::GetInstance()->Begin();

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
	Vector3 playerPosition = player_->GetTranslate();
	Vector3 playerRotation = player_->GetRotate();

	if (ImGui::DragFloat3("Player Position", &playerPosition.x, 0.01f)) {
		player_->SetTranslate(playerPosition);
	}
	if (ImGui::DragFloat3("Player Rotation", &playerRotation.x, 0.01f)) {
		player_->SetRotate(playerRotation);
	}

	Enemy* debugEnemy =
		enemySpawnController_->GetNearestEnemy(player_->GetTranslate());
	if (debugEnemy != nullptr) {
		Vector3 enemyPosition = debugEnemy->GetTranslate();
		Vector3 enemyRotation = debugEnemy->GetRotate();

		if (ImGui::DragFloat3("Enemy Position", &enemyPosition.x, 0.01f)) {
			debugEnemy->SetTranslate(enemyPosition);
		}
		if (ImGui::DragFloat3("Enemy Rotation", &enemyRotation.x, 0.01f)) {
			debugEnemy->SetRotate(enemyRotation);
		}
	}

	// camera
	Vector3 cameraPos = camera_->GetTranslate();

	if (ImGui::DragFloat3("Camera Position", &cameraPos.x, 0.01f)) {
		camera_->SetTranslate(cameraPos);
	}

	Vector3 cameraRotate = camera_->GetRotate();

	if (ImGui::DragFloat3("Camera Rotation", &cameraRotate.x, 0.01f)) {
		camera_->SetRotate(cameraRotate);
	}

	// デモウィンドウの表示
	// ゲーム中のデモウィンドウは表示しない。

	//ImGuiManager::GetInstance()->End();
	}

#endif

	emitter_->Update();

	if (railCamera_->IsActive()) {
		constexpr float kRailMoveSpeed = 0.08f;
		if (input_->PushKey(DIK_A)) {
			railPlayerOffset_.x -= kRailMoveSpeed;
		}
		if (input_->PushKey(DIK_D)) {
			railPlayerOffset_.x += kRailMoveSpeed;
		}
		if (input_->PushKey(DIK_W)) {
			railPlayerOffset_.y += kRailMoveSpeed;
		}
		if (input_->PushKey(DIK_S)) {
			railPlayerOffset_.y -= kRailMoveSpeed;
		}

		railPlayerOffset_.x = std::clamp(railPlayerOffset_.x, -3.5f, 3.5f);
		railPlayerOffset_.y = std::clamp(railPlayerOffset_.y, -1.5f, 2.5f);

		railCamera_->Update();
		const Vector3 railPosition = railCamera_->GetRailPosition();
		const Vector3 railRight = railCamera_->GetRight();
		const Vector3 playerPosition = {
			railPosition.x + railRight.x * railPlayerOffset_.x,
			railPosition.y + railPlayerOffset_.y,
			railPosition.z + railRight.z * railPlayerOffset_.x
		};
		player_->SetTranslate(playerPosition);
		player_->SetRotate(railCamera_->GetRailRotation());
		player_->UpdateTransform();
	}
	else {
		player_->Update(input_);
		Vector3 position = player_->GetTranslate();
		position.x = std::clamp(position.x, -12.0f, 12.0f);
		position.z = std::clamp(position.z, -4.0f, 30.0f);
		player_->SetTranslate(position);
		player_->UpdateTransform();
		UpdateFollowCamera();
	}

	enemySpawnController_->Update(player_->GetTranslate());
	skydome_->Update();
	reticle3D_->Update(*camera_);
	lockOnTarget_ = nullptr;
	if (reticle3D_->IsVisible()) {
		lockOnTarget_ = enemySpawnController_->GetLockOnTarget(
			*camera_,
			reticle3D_->GetScreenPosition(),
			70.0f
		);
	}
	reticle3D_->SetLocked(lockOnTarget_ != nullptr);

	if (input_->PushKey(DIK_SPACE) && shotCooldown_ == 0) {
		SpawnPlayerBullet();
		shotCooldown_ = 12;
	}

	++enemyBulletTimer_;
	if (enemyBulletTimer_ >= 90) {
		enemyBulletTimer_ = 0;
		SpawnEnemyBullet();
	}

	UpdateProjectiles();
	CheckProjectileCollisions();

	// 命中で対象が削除された場合は、同じフレームでロック表示も解除する。
	if (enemySpawnController_->ContainsEnemy(lockOnTarget_)) {
		Vector3 targetPosition = lockOnTarget_->GetTranslate();
		// 機体の原点をロックオン中心にする。
		reticle3D_->UpdateLockMarker(*camera_, &targetPosition);
	}
	else {
		lockOnTarget_ = nullptr;
		reticle3D_->UpdateLockMarker(*camera_, nullptr);
	}

	isPlayerEnemyColliding_ =
		enemySpawnController_->IsCollision(player_->GetCollider());

	if (isPlayerEnemyColliding_) { mission_.Damage(); }
	mission_.Resolve();

#ifdef USE_IMGUI
	if (showDebug_) {
	ImGui::Text(
		"Camera: %s (R key)",
		railCamera_->IsActive() ? "CUBIC RAIL" : "THIRD PERSON"
	);
	ImGui::Text("Player Shot: SPACE");
	ImGui::Text("3D Reticle: Mouse Cursor");
	ImGui::Text(
		"Shot Type: %s",
		lockOnTarget_ != nullptr ? "HOMING MISSILE" : "NORMAL BULLET"
	);
	ImGui::Text(
		"Enemies: %d",
		static_cast<int>(enemySpawnController_->GetEnemyCount())
	);
	ImGui::Text(
		"Enemy Spawn Frame: %d",
		enemySpawnController_->GetElapsedFrames()
	);
	ImGui::Text("Player Bullets: %d", static_cast<int>(playerBullets_.size()));
	ImGui::Text("Enemy Bullets: %d", static_cast<int>(enemyBullets_.size()));
	ImGui::Text(
		"Player vs Enemy: %s",
		isPlayerEnemyColliding_ ? "HIT" : "NONE"
	);
	}
#endif

	for (auto& sprite : sprites_) {
		//sprite->Update();
	}

	particleManager_->Update();
}

void GamePlayScene::Draw() {
	if (mission_.GetPhase() != MissionState::Phase::Playing) {
		spriteManager_->SetCommonRenderState();
		DrawGameUi();
		return;
	}

	auto cmdList = graphicsDevice_->GetCommandList();

	// 1. 状態を「レンダーターゲット」へ遷移
	renderTexture_->TransitionToRenderTarget(cmdList.Get());

	// 2. 描画先をレンダーテクスチャのRTVに切り替える
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = renderTexture_->GetRtvCPUHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = graphicsDevice_->GetDsvHandle();

	cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// 3. レンダーテクスチャのクリア（スライド通り、設定した赤色等でクリアされる）
	float clearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; // 初期化時の色と合わせる
	cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 天球を最初に描画する
	SrvManager::GetInstance()->PreDraw();
	skydome_->Draw();

	// === 3Dオブジェクト描画 ===
	object3DManager_->SetCommonRenderState();
	if (!mission_.IsInvincible() || (uiFrames_ / 5) % 2 == 0) {
		player_->Draw(skinCluster_);
	}
	enemySpawnController_->Draw(skinCluster_);


#ifdef _DEBUG
	if (showDebug_) {

	Matrix4x4 viewProjectionMatrix = Multiply(
		camera_->GetViewMatrix(),
		camera_->GetProjectionMatrix()
	);

	const Vector4 playerColliderColor = isPlayerEnemyColliding_
		? Vector4{ 1.0f, 1.0f, 0.0f, 1.0f }
	: Vector4{ 0.0f, 1.0f, 0.0f, 1.0f };
	const Vector4 enemyColliderColor = isPlayerEnemyColliding_
		? Vector4{ 1.0f, 1.0f, 0.0f, 1.0f }
	: Vector4{ 1.0f, 0.0f, 0.0f, 1.0f };

	player_->GetCollider().Draw(
		viewProjectionMatrix,
		1280.0f,
		720.0f,
		playerColliderColor
	);
	for (const auto& enemy : enemySpawnController_->GetEnemies()) {
		enemy->GetCollider().Draw(
			viewProjectionMatrix,
			1280.0f,
			720.0f,
			enemyColliderColor
		);
	}

	for (const auto& bullet : playerBullets_) {
		bullet->GetCollider().Draw(
			viewProjectionMatrix,
			1280.0f,
			720.0f,
			{ 0.0f, 0.5f, 1.0f, 1.0f }
		);
	}
	for (const auto& bullet : enemyBullets_) {
		bullet->GetCollider().Draw(
			viewProjectionMatrix,
			1280.0f,
			720.0f,
			{ 1.0f, 0.2f, 0.1f, 1.0f }
		);
	}

	railCamera_->DrawDebug(
		viewProjectionMatrix,
		1280.0f,
		720.0f
	);
	Vector3 reticleDebugOrigin = player_->GetTranslate();
	// 機体の中心から照準への線を表示する。
	reticle3D_->DrawDebug(
		viewProjectionMatrix,
		reticleDebugOrigin,
		1280.0f,
		720.0f
	);

	}
#endif

	// === パーティクル描画 ===
	SrvManager::GetInstance()->PreDraw();

	particleManager_->SetCommonRenderState();
	particleManager_->Draw();

	// 1. 描き込みが終わったので、状態を「シェーダーリソース（読み込み用）」へ遷移
	renderTexture_->TransitionToShaderResource(cmdList.Get());

	// 2. 描画先を「いつものバックバッファ」に戻す
	graphicsDevice_->SetBackBufferAsRenderTarget();

	cmdList->SetGraphicsRootSignature(copyRootSignature_.Get());
	cmdList->SetPipelineState(copyPipelineState_.Get());

	// プリミティブトポロジーを三角形に設定
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// レンダーテクスチャの SRV（GPUハンドル）をシェーダーの register(t0) にバインド [cite: 2]
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = renderTexture_->GetSrvGPUHandle();
	cmdList->SetGraphicsRootDescriptorTable(0, srvGpuHandle);

	// 頂点3つで全画面に描画 
	cmdList->DrawInstanced(3, 1, 0, 0);

	// === スプライト描画 ===
	spriteManager_->SetCommonRenderState();
	for (auto& bullet : playerBullets_) {
		bullet->Draw();
	}
	for (auto& bullet : enemyBullets_) {
		bullet->Draw();
	}
	reticle3D_->Draw();
	DrawGameUi();

}

void GamePlayScene::Finalize() {
	for (auto& screen : screenImages_) { screen.reset(); }
	gameUi_.reset();
	sound_->Unload(&bgmData_);

	delete input_;
	delete sound_;
	railCamera_.reset();
	delete camera_;
	camera_ = nullptr;
	skydome_.reset();
	playerBullets_.clear();
	enemyBullets_.clear();
	reticle3D_.reset();
	player_.reset();
	enemySpawnController_.reset();
	delete object3DManager_;

	for (Sprite* sprite : sprites_) {
		delete sprite;
	}
	sprites_.clear();

	delete spriteManager_;
	delete emitter_;
	delete particleManager_;
}

GamePlayScene::GamePlayScene() {}

GamePlayScene::~GamePlayScene() {}

void GamePlayScene::StartMission()
{
	// GPUオブジェクトや画像は共有し、再挑戦では進行データだけを初期化する。
	mission_.Start();
	playerBullets_.clear();
	enemyBullets_.clear();
	lockOnTarget_ = nullptr;
	enemySpawnController_->Reset();
	enemyBulletTimer_ = 0;
	shotCooldown_ = 0;
	isPlayerEnemyColliding_ = false;
	showDebug_ = false;
	railCamera_->Reset();
	railCamera_->SetActive(true);
	railPlayerOffset_ = { 0.0f, 0.0f };
	// 開始直後から、カメラと自機をレールの始点に揃える。
	railCamera_->Update();
	player_->SetTranslate(railCamera_->GetRailPosition());
	player_->SetRotate(railCamera_->GetRailRotation());
	player_->UpdateTransform();
	skydome_->Update();
	reticle3D_->Update(*camera_);
	reticle3D_->UpdateLockMarker(*camera_, nullptr);

}

void GamePlayScene::DrawGameUi()
{
	const Vector4 white{ 0.92f, 0.96f, 1.0f, 1.0f };
	const Vector4 muted{ 0.5f, 0.65f, 0.78f, 1.0f };
	const Vector4 orange{ 1.0f, 0.6f, 0.15f, 1.0f };
	const Vector4 cyan{ 0.2f, 0.9f, 0.9f, 1.0f };
	const Vector4 panel{ 0.025f, 0.045f, 0.08f, 0.94f };
	gameUi_->Begin();
	if (mission_.GetPhase() == MissionState::Phase::Playing) {
		// 未ロックの敵にも目印を出し、空の背景に埋もれないようにする。
		const auto& vp = camera_->GetViewProjectionMatrix();
		for (const auto& enemy : enemySpawnController_->GetEnemies()) {
			Vector3 p = enemy->GetTranslate();
			p.y += 2.0f * enemy->GetScale().y;
			const float w = p.x * vp.m[0][3] + p.y * vp.m[1][3] + p.z * vp.m[2][3] + vp.m[3][3];
			if (w <= 0.0f) { continue; }
			const float z = (p.x * vp.m[0][2] + p.y * vp.m[1][2] + p.z * vp.m[2][2] + vp.m[3][2]) / w;
			if (z < 0.0f || z > 1.0f) { continue; }
			const float x = ((p.x * vp.m[0][0] + p.y * vp.m[1][0] + p.z * vp.m[2][0] + vp.m[3][0]) / w + 1.0f) * 640.0f;
			const float y = (1.0f - (p.x * vp.m[0][1] + p.y * vp.m[1][1] + p.z * vp.m[2][1] + vp.m[3][1]) / w) * 360.0f;
			if (x < 40.0f || x > 1240.0f || y < 128.0f || y > 650.0f) { continue; }
			// ロック対象には既存のLOCK ON表示があるため二重に表示しない。
			if (enemy.get() == lockOnTarget_) { continue; }
			gameUi_->Rect(x - 34.0f, y - 26.0f, 68.0f, 24.0f, panel);
			gameUi_->Text("ENEMY", x - 27.0f, y - 25.0f, 1.2f, orange);
		}
		gameUi_->Rect(24, 20, 1232, 76, panel);
		gameUi_->Rect(24, 20, 4, 76, orange);
		gameUi_->Text("HP", 44, 42, 2, white);
		for (int i = 0; i < MissionState::kMaxHealth; ++i) {
			gameUi_->Rect(104.0f + float(i) * 38.0f, 46, 28, 22,
				i < mission_.GetHealth() ? cyan : Vector4{ 0.15f, 0.2f, 0.25f, 1.0f });
		}
		gameUi_->Text("TARGETS " + std::to_string(mission_.GetDefeated()) + "/5", 332, 42, 2, white);
		gameUi_->Text("TIME " + std::to_string(mission_.GetRemainingSeconds()), 668, 42, 2,
			mission_.GetRemainingSeconds() <= 15 ? orange : white);
		gameUi_->Text(lockOnTarget_ ? "LOCK ON" : "SEARCH", 1040, 44, 1.5f, lockOnTarget_ ? orange : muted);
		gameUi_->Rect(24, 666, 1232, 34, panel);
		gameUi_->Text("WASD MOVE   MOUSE AIM   HOLD SPACE FIRE   ESC TITLE   F1 DEBUG", 44, 671, 1.2f, white);
		if (mission_.IsInvincible()) {
			gameUi_->Rect(0, 0, 1280, 6, orange);
			gameUi_->Rect(0, 714, 1280, 6, orange);
		}
		return;
	}

	const auto phase = mission_.GetPhase();
	const size_t screenIndex = phase == MissionState::Phase::Title ? 0 :
		(phase == MissionState::Phase::Clear ? 1 : 2);
	screenImages_[screenIndex]->Draw();
	// 結果は画像内の操作説明と重ならない下端に表示する。
	if (phase != MissionState::Phase::Title) {
		gameUi_->CenteredText("TARGETS " + std::to_string(mission_.GetDefeated()) +
			" / 5    HP " + std::to_string(mission_.GetHealth()) + " / 3    TIME " +
			std::to_string(mission_.GetRemainingSeconds()) + "s", 652, 1.3f, white);
		if (phase == MissionState::Phase::GameOver) {
			gameUi_->CenteredText(mission_.GetHealth() == 0 ? "HP ZERO" : "TIME UP", 682, 1.0f, white);
		}
	}
}
void GamePlayScene::UpdateFollowCamera()
{
	const Vector3 playerPosition = player_->GetTranslate();
	const float playerYaw = player_->GetRotate().y;
	constexpr float kFollowDistance = 9.0f;
	constexpr float kFollowHeight = 4.0f;
	constexpr float kPitch = 0.35f;
	constexpr float kFollowRate = 0.12f;

	const Vector3 desiredCameraPosition = {
		playerPosition.x - std::sin(playerYaw) * kFollowDistance,
		playerPosition.y + kFollowHeight,
		playerPosition.z - std::cos(playerYaw) * kFollowDistance
	};
	const Vector3 cameraPosition = Lerp(
		camera_->GetTranslate(),
		desiredCameraPosition,
		kFollowRate
	);

	camera_->SetTranslate(cameraPosition);
	camera_->SetRotate({ kPitch, playerYaw, 0.0f });
	camera_->Update();
}

void GamePlayScene::SpawnPlayerBullet()
{
	Vector3 start = player_->GetTranslate();
	// 機体中心から発射する。

	const Vector3 target = reticle3D_->GetWorldPosition();
	const Vector3 directionVector = {
		target.x - start.x,
		target.y - start.y,
		target.z - start.z
	};
	const Vector3 direction = Normalize(directionVector);
	start.x += direction.x * 0.8f;
	start.y += direction.y * 0.8f;
	start.z += direction.z * 0.8f;

	if (lockOnTarget_ != nullptr &&
		enemySpawnController_->ContainsEnemy(lockOnTarget_)) {
		auto bullet = std::make_unique<HomingProjectile>();
		bullet->Initialize(
			spriteManager_,
			enemySpawnController_.get(),
			lockOnTarget_,
			start,
			{
				direction.x * 0.13f,
				direction.y * 0.13f,
				direction.z * 0.13f
			}
		);
		playerBullets_.push_back(std::move(bullet));
		return;
	}

	auto bullet = std::make_unique<NormalProjectile>();
	bullet->Initialize(
		spriteManager_,
		start,
		{
			direction.x * 0.15f,
			direction.y * 0.15f,
			direction.z * 0.15f
		}
	);
	playerBullets_.push_back(std::move(bullet));
}

void GamePlayScene::SpawnEnemyBullet()
{
	Enemy* shootingEnemy =
		enemySpawnController_->GetNearestEnemy(player_->GetTranslate());
	if (shootingEnemy == nullptr) {
		return;
	}

	Vector3 start = shootingEnemy->GetTranslate();
	// 敵機の中心から発射する。

	Vector3 target = player_->GetTranslate();
	// 自機の中心を狙う。

	const Vector3 directionVector = {
		target.x - start.x,
		target.y - start.y,
		target.z - start.z
	};
	const Vector3 direction = Normalize(directionVector);
	start.x += direction.x * 0.8f;
	start.y += direction.y * 0.8f;
	start.z += direction.z * 0.8f;

	auto bullet = std::make_unique<Projectile>();
	bullet->Initialize(
		spriteManager_,
		"Resources/circle2.png",
		ProjectileOwner::Enemy,
		start,
		{
			direction.x * 0.07f,
			direction.y * 0.07f,
			direction.z * 0.07f
		},
		{ 1.0f, 0.2f, 0.1f, 1.0f }
	);
	enemyBullets_.push_back(std::move(bullet));
}

void GamePlayScene::UpdateProjectiles()
{
	for (auto& bullet : playerBullets_) {
		bullet->Update(*camera_);
	}
	for (auto& bullet : enemyBullets_) {
		bullet->Update(*camera_);
	}
}

void GamePlayScene::CheckProjectileCollisions()
{
	for (auto& playerBullet : playerBullets_) {
		if (!playerBullet->IsActive()) {
			continue;
		}

		if (enemySpawnController_->HitEnemy(playerBullet->GetCollider())) {
			mission_.DefeatEnemy();
			playerBullet->Deactivate();
			continue;
		}

		for (auto& enemyBullet : enemyBullets_) {
			if (!enemyBullet->IsActive()) {
				continue;
			}

			if (playerBullet->GetCollider().IsCollision(enemyBullet->GetCollider())) {
				playerBullet->Deactivate();
				enemyBullet->Deactivate();
				break;
			}
		}
	}

	for (auto& enemyBullet : enemyBullets_) {
		if (enemyBullet->IsActive() &&
			enemyBullet->GetCollider().IsCollision(player_->GetCollider())) {
			enemyBullet->Deactivate();
			mission_.Damage();
		}
	}

	auto removeInactive = [](const std::unique_ptr<Projectile>& bullet) {
		return !bullet->IsActive();
		};

	playerBullets_.erase(
		std::remove_if(playerBullets_.begin(), playerBullets_.end(), removeInactive),
		playerBullets_.end()
	);
	enemyBullets_.erase(
		std::remove_if(enemyBullets_.begin(), enemyBullets_.end(), removeInactive),
		enemyBullets_.end()
	);
}

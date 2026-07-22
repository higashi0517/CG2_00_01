#include "GamePlayScene.h"
#include "SrvManager.h"
#include "RenderTexture.h"

void GamePlayScene::Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)
{
	winApp_ = winApp;
	graphicsDevice_ = graphicsDevice;

	// 3Dモデルマネジャの初期化
	ModelManager::GetInstance()->Initialize(graphicsDevice_);
	// .objモデルの読み込み
	ModelManager::GetInstance()->LoadModel("AnimatedCube.gltf");
	animation_ = LoadAnimationFile("./Resources", "AnimatedCube.gltf");

	input_ = new Input();
	input_->Initialize(winApp_);

	sound_ = new Sound();

	camera_ = new Camera();
	camera_->SetTranslate({ 0.0f, 20.0f, -40.0f });
	camera_->SetRotate({ 0.42f, 0.0f, 0.0f });

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

	object3D_ = new Object3D();
	object3D_->Initialize(object3DManager_);
	object3D_->SetModel("AnimatedCube.gltf");
	object3D_->SetAnimation(&animation_);

	object3D_2_ = new Object3D();
	object3D_2_->Initialize(object3DManager_);
	object3D_2_->SetModel("terrain.obj");

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

	// ★重要: 頂点バッファを使わないため、InputLayout は空にする
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

	// 【修正1】サンプルマスクを適切に設定する (0 のままだと何も描画されません)
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // もしくは 0xFFFFFFFF

	// レンダーターゲットの設定
	psoDesc.NumRenderTargets = 1;

	// 【修正2】フォーマットをバックバッファに合わせて _SRGB に変更する
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// ★修正: コメントアウトを解除し、実際にPipelineStateオブジェクトを生成
	hr = graphicsDevice_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&copyPipelineState_));
	assert(SUCCEEDED(hr));
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

	Vector3 cameraRotate = camera_->GetRotate();

	if (ImGui::DragFloat3("Camera Rotation", &cameraRotate.x, 0.01f)) {
		camera_->SetRotate(cameraRotate);
	}

	// デモウィンドウの表示
	ImGui::ShowDemoWindow();

	//ImGuiManager::GetInstance()->End();

#endif

	// カメラの更新
		camera_->Update();

	emitter_->Update();

	object3D_->Update();
	//object3D_2_->Update();

	for (auto& sprite : sprites_) {
		//sprite->Update();
	}

	//particleManager_->Update();
}

void
GamePlayScene::Draw() {

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

	// === 3Dオブジェクト描画 ===
	object3DManager_->SetCommonRenderState();
	object3D_->Draw();
	// object3D_2_->Draw();


	// === パーティクル描画 ===
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
	for (auto& sprite : sprites_) {
		sprite->Draw();
	}

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

GamePlayScene::GamePlayScene() {}

GamePlayScene::~GamePlayScene() {}

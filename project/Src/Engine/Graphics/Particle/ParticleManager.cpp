#include "ParticleManager.h"
#include <cassert>
#include <GraphicsDevice.h>
#include <SrvManager.h>
#include <TextureManager.h>
#include "Camera.h"
#include <numbers>

void ParticleManager::Initialize(GraphicsDevice* graphicsDevice_) {
	this->graphicsDevice = graphicsDevice_;

	const uint32_t kCylinderDivide = 32;
	const uint32_t kVertexCount = kCylinderDivide * 6;

	// バッファサイズを 4 から kVertexCount(192) に変更
	vertexResource = graphicsDevice->CreateBufferResource(sizeof(VertexData) * kVertexCount);
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(VertexData) * kVertexCount;
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	VertexData* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	const float kTopRadius = 1.0f;
	const float kBottomRadius = 1.0f;
	const float kHeight = 3.0f;
	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kCylinderDivide);

	for (uint32_t index = 0; index < kCylinderDivide; ++index) {
		float sin = std::sin(index * radianPerDivide);
		float cos = std::cos(index * radianPerDivide);
		float sinNext = std::sin((index + 1) * radianPerDivide);
		float cosNext = std::cos((index + 1) * radianPerDivide);

		float u = float(index) / float(kCylinderDivide);
		float uNext = float(index + 1) / float(kCylinderDivide);

		// TRIANGLELIST（三角形リスト）用に、1ループで6頂点ずつ直接格納していく
		uint32_t baseIndex = index * 6;

		// 1つ目の三角形
		vertexData[baseIndex + 0] = { {-sin * kTopRadius, kHeight, cos * kTopRadius, 1.0f}, {u, 0.0f}, {-sin, 0.0f, cos} };
		vertexData[baseIndex + 1] = { {-sinNext * kTopRadius, kHeight, cosNext * kTopRadius, 1.0f}, {uNext, 0.0f}, {-sinNext, 0.0f, cosNext} };
		vertexData[baseIndex + 2] = { {-sin * kBottomRadius, 0.0f, cos * kBottomRadius, 1.0f}, {u, 1.0f}, {-sin, 0.0f, cos} };

		// 2つ目の三角形
		vertexData[baseIndex + 3] = { {-sin * kBottomRadius, 0.0f, cos * kBottomRadius, 1.0f}, {u, 1.0f}, {-sin, 0.0f, cos} };
		vertexData[baseIndex + 4] = { {-sinNext * kTopRadius, kHeight, cosNext * kTopRadius, 1.0f}, {uNext, 0.0f}, {-sinNext, 0.0f, cosNext} };
		vertexData[baseIndex + 5] = { {-sinNext * kBottomRadius, 0.0f, cosNext * kBottomRadius, 1.0f}, {uNext, 1.0f}, {-sinNext, 0.0f, cosNext} };
	}

	materialResource = graphicsDevice->CreateBufferResource(sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	// デフォルトの設定
	materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData->enableLighting = 0;
	materialData->uvTransform = MakeIdentity4x4();

	randomEngine_ = std::mt19937(seedGenerator_());

	CreateRootSignature();
	CreateGraphicsPipelineState();
}

void ParticleManager::SetCommonRenderState() {
	auto commandList = graphicsDevice->GetCommandList();
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->SetPipelineState(graphicsPipelineState.Get());
	// 頂点バッファを使わず四角形を描画するので TriangleStrip にします
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ParticleManager::CreateRootSignature() {
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[2] = {};
	// RootParameter 0: テクスチャ用 (t0)
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParameter 1: インスタンシングデータ用 (t1)
	descriptorRange[1].BaseShaderRegister = 0;
	descriptorRange[1].NumDescriptors = 1;
	descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[3] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &descriptorRange[1];
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;

	// RootParameter 2: マテリアル用定数バッファ (b0)
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // 定数バッファ(CBV)を指定
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダーからアクセス
	rootParameters[2].Descriptor.ShaderRegister = 0; // レジスタ番号 b0

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	graphicsDevice->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
}

void ParticleManager::CreateGraphicsPipelineState() {
	auto vertexShaderBlob = graphicsDevice->CompileShader(L"Resources/shaders/Particle.VS.hlsl", L"vs_6_0");
	auto pixelShaderBlob = graphicsDevice->CompileShader(L"Resources/shaders/Particle.PS.hlsl", L"ps_6_0");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSignature.Get();

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	desc.InputLayout.pInputElementDescs = inputElementDescs;
	desc.InputLayout.NumElements = _countof(inputElementDescs);


	desc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	desc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

	// ブレンドステート（加算合成などが必要な場合はここを変えます）
	desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	desc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 両面描画
	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

	// パーティクルはZバッファへの書き込みをオフにするのが定石です
	desc.DepthStencilState.DepthEnable = true;
	desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleDesc.Count = 1;
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	graphicsDevice->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&graphicsPipelineState));
}

void ParticleManager::CreateParticleGroup(const std::string name, const std::string textureFilePath)
{
	// 登録済みの名前かチェックしてassert
	assert(particleGroups.find(name) == particleGroups.end());

	// 新たな空のパーティクルグループを作成し、コンテナに登録
	ParticleGroup& newGroup = particleGroups[name];

	// マテリアルデータにテクスチャファイルパスを設定
	newGroup.textureFilePath = textureFilePath;

	// テクスチャを読み込む / テクスチャのSRVインデックスを記録
	TextureManager::GetInstance()->LoadTexture(textureFilePath);
	newGroup.textureSrvIndex = TextureManager::GetInstance()->GetSrvIndex(textureFilePath);

	// インスタンス数の最大値を設定
	newGroup.instanceCount = 0;

	// インスタンシング用リソースの生成
	newGroup.instancingResource = graphicsDevice->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);

	// 生成したリソースをMapして、データを書き込むためのポインタを取得
	newGroup.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&newGroup.mappedData));

	// 初期値として単位行列を書き込む
	for (uint32_t i = 0; i < kNumMaxInstance; ++i) {
		newGroup.mappedData[i].WVP = MakeIdentity4x4();
		newGroup.mappedData[i].World = MakeIdentity4x4();
	}

	// インスタンシング用にSRVを確保してSRVインデックスを記録
	newGroup.instancingSrvIndex = SrvManager::GetInstance()->Allocate();

	// SRV生成 (StructuredBuffer用設定)
	SrvManager::GetInstance()->CreateSRVforStructuredBuffer(
		newGroup.instancingSrvIndex,
		newGroup.instancingResource.Get(),
		kNumMaxInstance,
		sizeof(ParticleForGPU)
	);
}

void ParticleManager::Update()
{
	// 1. ビルボード行列の計算
	// カメラのワールド行列（ビュー行列の逆行列）を取得
	Matrix4x4 cameraMatrix = Inverse(camera->GetViewMatrix());
	// 板ポリを裏返すための回転行列（Y軸180度回転）
	Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);

	// ビルボード行列 = 回転 * カメラ行列
	Matrix4x4 billboardMatrix = Multiply(backToFrontMatrix, cameraMatrix);

	// 平行移動成分を無効化する
	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

	// カメラのビュー・プロジェクション行列を取得
	Matrix4x4 viewMatrix = camera->GetViewMatrix();
	Matrix4x4 projectionMatrix = camera->GetProjectionMatrix();
	Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);

	for (auto& [name, group] : particleGroups)
	{
		uint32_t numInstance = 0;
		for (auto it = group.particles.begin(); it != group.particles.end(); )
		{
			if (it->currentTime >= it->lifeTime) {
				it = group.particles.erase(it);
				continue;
			}

			//if (numInstance >= kNumMaxInstance) break;

			// 座標と寿命の更新
			it->position.x += it->velocity.x;
			it->position.y += it->velocity.y;
			it->position.z += it->velocity.z;
			it->currentTime += 1.0f / 60.0f;
			float alpha = 1.0f - (it->currentTime / it->lifeTime);
			it->color.w = alpha;

			if (numInstance < kNumMaxInstance) {
				// 2. ワールド行列の計算（資料の通り Scale * Billboard * Translate）
				Matrix4x4 scaleMatrix = MakeScaleMatrix(it->scale);
				Matrix4x4 rotateZMatrix = MakeRotateZMatrix(it->rotate.z);
				Matrix4x4 translateMatrix = MakeTranslateMatrix(it->position);

				// 行列の合成
				Matrix4x4 worldMatrix = Multiply(scaleMatrix, rotateZMatrix);
				worldMatrix = Multiply(worldMatrix, billboardMatrix);
				worldMatrix = Multiply(worldMatrix, translateMatrix);

				Matrix4x4 wvpMatrix = Multiply(worldMatrix, viewProjectionMatrix);

				group.mappedData[numInstance].WVP = wvpMatrix;
				group.mappedData[numInstance].World = worldMatrix;
				group.mappedData[numInstance].color = it->color;

				numInstance++;
			}
			++it;
		}
		group.instanceCount = numInstance;

	}
}
void ParticleManager::Draw()
{
	graphicsDevice->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	graphicsDevice->GetCommandList()->SetGraphicsRootConstantBufferView(2, materialResource->GetGPUVirtualAddress());

	for (auto& [name, group] : particleGroups)
	{
		if (group.instanceCount == 0) continue;

		SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(0, group.textureSrvIndex);
		SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(1, group.instancingSrvIndex);
		graphicsDevice->GetCommandList()->DrawInstanced(32 * 6, group.instanceCount, 0, 0);
	}
}

void ParticleManager::Emit(const std::string& name, const Vector3& position, uint32_t count, float minScaleY, float maxScaleY) {

	assert(particleGroups.find(name) != particleGroups.end());

	ParticleGroup& group = particleGroups[name];

	for (uint32_t i = 0; i < count; ++i) {
		// [[修正ポイント]] MakeNewParticle を呼び出してランダムな値をセットしたパーティクルを受け取る
		Particle particle = MakeNewParticle(randomEngine_, position, minScaleY, maxScaleY);

		// リストに追加
		group.particles.push_back(particle);
	}
}

ParticleManager::Particle ParticleManager::MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate, float minScaleY, float maxScaleY) {
	// 乱数の範囲設定
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distVelocity(0.0f, 0.0f);
	std::uniform_real_distribution<float> distTime(5.0f, 10.0f);
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);

	std::uniform_real_distribution<float> distRotate(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> distScale(minScaleY, maxScaleY);

	ParticleManager::Particle particle;


	particle.velocity = {
		distVelocity(randomEngine),
		distVelocity(randomEngine),
		distVelocity(randomEngine)
	};

	particle.position = {
	translate.x,
	translate.y,
	translate.z
	};

	/*particle.color = {
		distColor(randomEngine),
		distColor(randomEngine),
		distColor(randomEngine),
		1.0f
	};*/

	// 白
	particle.color = { 1.0f, 1.0f, 1.0f, 1.0f };

	//particle.scale = { 0.05f, distScale(randomEngine), 1.0f };
	particle.scale = { 1.0f,1.0f,1.0f };
	//particle.rotate = { 0.0f, 0.0f, distRotate(randomEngine) };
	particle.rotate = { 0.0f, 0.0f, 0.0f };

	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0.0f;

	return particle;
}
#include "Skybox.h"
#include <cassert>
#include <GraphicsDevice.h>
#include <SrvManager.h>
#include <TextureManager.h>
#include "Camera.h"
#include <numbers>

void Skybox::Initialize(GraphicsDevice* graphicsDevice_, SrvManager* srvManager_, const char* texturePath) {

	this->graphicsDevice = graphicsDevice_;

	TextureManager::GetInstance()->LoadTexture(texturePath);
	this->textureIndex = TextureManager::GetInstance()->GetSrvIndex(texturePath);

	vertexResource = graphicsDevice->CreateBufferResource(sizeof(VertexData) * 36);
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 36;
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	VertexData* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// 前面 (Front, +Z)
	vertexData[0].position = { -1.0f,  1.0f,  1.0f, 1.0f };
	vertexData[1].position = { 1.0f,  1.0f,  1.0f, 1.0f };
	vertexData[2].position = { -1.0f, -1.0f,  1.0f, 1.0f };
	vertexData[3].position = { 1.0f,  1.0f,  1.0f, 1.0f };
	vertexData[4].position = { 1.0f, -1.0f,  1.0f, 1.0f };
	vertexData[5].position = { -1.0f, -1.0f,  1.0f, 1.0f };

	// 背面 (Back, -Z)
	vertexData[6].position = { 1.0f,  1.0f, -1.0f, 1.0f };
	vertexData[7].position = { -1.0f,  1.0f, -1.0f, 1.0f };
	vertexData[8].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[9].position = { -1.0f,  1.0f, -1.0f, 1.0f };
	vertexData[10].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[11].position = { 1.0f, -1.0f, -1.0f, 1.0f };

	// 左面 (Left, -X)
	vertexData[12].position = { -1.0f,  1.0f, -1.0f, 1.0f };
	vertexData[13].position = { -1.0f,  1.0f,  1.0f, 1.0f };
	vertexData[14].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[15].position = { -1.0f,  1.0f,  1.0f, 1.0f };
	vertexData[16].position = { -1.0f, -1.0f,  1.0f, 1.0f };
	vertexData[17].position = { -1.0f, -1.0f, -1.0f, 1.0f };

	// 右面 (Right, +X)
	vertexData[18].position = { 1.0f,  1.0f,  1.0f, 1.0f };
	vertexData[19].position = { 1.0f,  1.0f, -1.0f, 1.0f };
	vertexData[20].position = { 1.0f, -1.0f,  1.0f, 1.0f };
	vertexData[21].position = { 1.0f,  1.0f, -1.0f, 1.0f };
	vertexData[22].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[23].position = { 1.0f, -1.0f,  1.0f, 1.0f };

	// 上面 (Top, +Y)
	vertexData[24].position = { -1.0f,  1.0f, -1.0f, 1.0f };
	vertexData[25].position = { 1.0f,  1.0f, -1.0f, 1.0f };
	vertexData[26].position = { -1.0f,  1.0f,  1.0f, 1.0f };
	vertexData[27].position = { 1.0f,  1.0f, -1.0f, 1.0f };
	vertexData[28].position = { 1.0f,  1.0f,  1.0f, 1.0f };
	vertexData[29].position = { -1.0f,  1.0f,  1.0f, 1.0f };

	// 下面 (Bottom, -Y)
	vertexData[30].position = { -1.0f, -1.0f,  1.0f, 1.0f };
	vertexData[31].position = { 1.0f, -1.0f,  1.0f, 1.0f };
	vertexData[32].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[33].position = { 1.0f, -1.0f,  1.0f, 1.0f };
	vertexData[34].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	vertexData[35].position = { -1.0f, -1.0f, -1.0f, 1.0f };

	constBuffer = graphicsDevice->CreateBufferResource(sizeof(ConstBufferData));
	constBuffer->Map(0, nullptr, reinterpret_cast<void**>(&constMap));
	constMap->viewProjection = MakeIdentity4x4();

	// 行列のバッファ作成の後に追加
	materialResource = graphicsDevice->CreateBufferResource(sizeof(MaterialData));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // RGBA: 白色にしておく

	CreateRootSignature();
	CreateGraphicsPipelineState();
}
void Skybox::SetCommonRenderState() {
	auto commandList = graphicsDevice->GetCommandList();
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->SetPipelineState(graphicsPipelineState.Get());
	// 頂点バッファを使わず四角形を描画するので TriangleStrip にします
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

void Skybox::CreateRootSignature() {
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// テクスチャ(t0)用の設定
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// パラメータは2つだけにする
	D3D12_ROOT_PARAMETER rootParameters[3] = {};

	// 0番目: 定数バッファ (CBV, b0)
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[0].Descriptor.ShaderRegister = 0; // b0

	// 1番目: テクスチャ (DESCRIPTOR_TABLE, t0)
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].Descriptor.ShaderRegister = 0; // b0

	// サンプラーの設定 (そのまま)
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
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
void Skybox::CreateGraphicsPipelineState() {
	auto vertexShaderBlob = graphicsDevice->CompileShader(L"Resources/shaders/Skybox.VS.hlsl", L"vs_6_0");
	auto pixelShaderBlob = graphicsDevice->CompileShader(L"Resources/shaders/Skybox.PS.hlsl", L"ps_6_0");

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


void Skybox::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
	// 1. 引数で受け取ったビュー行列を書き換え用にコピーする
	Matrix4x4 skyboxViewMatrix = viewMatrix;

	// 2. 平行移動（カメラの位置）の成分を強制的にゼロにして、回転成分だけにする
	skyboxViewMatrix.m[3][0] = 0.0f; // X軸の移動を消す
	skyboxViewMatrix.m[3][1] = 0.0f; // Y軸の移動を消す
	skyboxViewMatrix.m[3][2] = 0.0f; // Z軸の移動を消す

	// 3. 移動成分を消したビュー行列を使って WVP行列（今回はVP行列）を計算する
	constMap->viewProjection = Multiply(skyboxViewMatrix, projectionMatrix);
}

void Skybox::Draw() {
	auto commandList = graphicsDevice->GetCommandList();
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->SetPipelineState(graphicsPipelineState.Get());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	// 0番目に定数バッファ(行列)をセット
	commandList->SetGraphicsRootConstantBufferView(0, constBuffer->GetGPUVirtualAddress());

	// 1番目にテクスチャをセットする
	SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(1, this->textureIndex);

	// 2番目にマテリアルをセット
	commandList->SetGraphicsRootConstantBufferView(2, materialResource->GetGPUVirtualAddress());

	// 描画
	commandList->DrawInstanced(36, 1, 0, 0);
}
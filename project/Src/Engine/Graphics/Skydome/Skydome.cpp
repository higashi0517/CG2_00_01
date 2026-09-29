#include "Skydome.h"
#include "Camera.h"
#include "GraphicsDevice.h"
#include "Model.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include <cassert>
#include <cstring>
#include <cstddef>

void Skydome::Initialize(GraphicsDevice* graphicsDevice, Camera* camera)
{
	assert(graphicsDevice && camera);
	graphicsDevice_ = graphicsDevice;
	camera_ = camera;
	// OBJとMTLの相対パスは、どちらもskydomeフォルダを基準に解決する。
	const auto model = Model::LoadModelFile("Resources/skydome", "SkyDome.obj");
	assert(!model.vertices.empty() && !model.indices.empty());
	TextureManager::GetInstance()->LoadTexture(model.material.textureFilePath);
	textureIndex_ = TextureManager::GetInstance()->GetSrvIndex(model.material.textureFilePath);

	const UINT vertexBytes = static_cast<UINT>(model.vertices.size() * sizeof(Model::VertexData));
	vertexResource_ = graphicsDevice_->CreateBufferResource(vertexBytes);
	vertexView_ = { vertexResource_->GetGPUVirtualAddress(), vertexBytes, sizeof(Model::VertexData) };
	void* mapped = nullptr;
	HRESULT hr = vertexResource_->Map(0, nullptr, &mapped);
	assert(SUCCEEDED(hr));
	std::memcpy(mapped, model.vertices.data(), vertexBytes);
	vertexResource_->Unmap(0, nullptr);

	indexCount_ = static_cast<uint32_t>(model.indices.size());
	const UINT indexBytes = indexCount_ * sizeof(uint32_t);
	indexResource_ = graphicsDevice_->CreateBufferResource(indexBytes);
	indexView_ = { indexResource_->GetGPUVirtualAddress(), indexBytes, DXGI_FORMAT_R32_UINT };
	hr = indexResource_->Map(0, nullptr, &mapped);
	assert(SUCCEEDED(hr));
	std::memcpy(mapped, model.indices.data(), indexBytes);
	indexResource_->Unmap(0, nullptr);

	matrixResource_ = graphicsDevice_->CreateBufferResource(sizeof(Matrix4x4));
	hr = matrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&viewProjection_));
	assert(SUCCEEDED(hr));
	CreatePipeline();
	Update();
}

void Skydome::CreatePipeline()
{
	D3D12_DESCRIPTOR_RANGE range{};
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range.NumDescriptors = 1;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	D3D12_ROOT_PARAMETER parameters[2]{};
	parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	parameters[1].DescriptorTable = { 1, &range };
	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	D3D12_ROOT_SIGNATURE_DESC root{};
	root.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	root.NumParameters = 2;
	root.pParameters = parameters;
	root.NumStaticSamplers = 1;
	root.pStaticSamplers = &sampler;
	Microsoft::WRL::ComPtr<ID3DBlob> blob, errors;
	HRESULT hr = D3D12SerializeRootSignature(&root, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errors);
	if (errors) { OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer())); }
	assert(SUCCEEDED(hr));
	hr = graphicsDevice_->GetDevice()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));

	const auto vs = graphicsDevice_->CompileShader(L"Resources/shaders/Skydome.VS.hlsl", L"vs_6_0");
	const auto ps = graphicsDevice_->CompileShader(L"Resources/shaders/Skydome.PS.hlsl", L"ps_6_0");
	const D3D12_INPUT_ELEMENT_DESC elements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(Model::VertexData, position)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, static_cast<UINT>(offsetof(Model::VertexData, texcord)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSignature_.Get();
	desc.InputLayout = { elements, 2 };
	desc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
	desc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
	desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	// 内側から球を見る。モデルの面の向きに依存させない。
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	desc.RasterizerState.DepthClipEnable = TRUE;
	desc.DepthStencilState.DepthEnable = TRUE;
	desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleDesc.Count = 1;
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	hr = graphicsDevice_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}

void Skydome::Update()
{
	if (!camera_ || !viewProjection_) { return; }
	// 平行移動を除き、カメラを常に球体の中心に置く。
	Matrix4x4 view = camera_->GetViewMatrix();
	view.m[3][0] = view.m[3][1] = view.m[3][2] = 0.0f;
	*viewProjection_ = Multiply(view, camera_->GetProjectionMatrix());
}

void Skydome::Draw()
{
	auto commandList = graphicsDevice_->GetCommandList();
	SrvManager::GetInstance()->PreDraw();
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pipelineState_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexView_);
	commandList->IASetIndexBuffer(&indexView_);
	commandList->SetGraphicsRootConstantBufferView(0, matrixResource_->GetGPUVirtualAddress());
	SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(1, textureIndex_);
	commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}
#include "SrvManager.h"
#include "GraphicsDevice.h"

const uint32_t SrvManager::kMaxSRVCount = 512;

void SrvManager::Initialize(GraphicsDevice* graphicsDevice)
{
	this->graphicsDevice = graphicsDevice;

	// デスクリプタヒープの生成
	DescriptorHeap = graphicsDevice->CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		kMaxSRVCount,
		true
	);
	// デスクリプタサイズの取得
	descriptorSize = graphicsDevice->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

uint32_t SrvManager::Allocate()
{
	// 返す番号を記憶
	int index = useIndex;
	// 次に使用するインデックスを更新
	useIndex++;
	// 上で記憶した番号を返す
	return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += (descriptorSize * index);
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += (descriptorSize * index);
	return handle;
}

void SrvManager::CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT MipLevels)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = MipLevels;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	graphicsDevice->GetDevice()->CreateShaderResourceView(
		pResource,
		&srvDesc,
		GetCPUDescriptorHandle(srvIndex)
	);
}

void SrvManager::CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = numElements;
	srvDesc.Buffer.StructureByteStride = structureByteStride;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	graphicsDevice->GetDevice()->CreateShaderResourceView(
		pResource,
		&srvDesc,
		GetCPUDescriptorHandle(srvIndex)
	);
}

void SrvManager::PreDraw()
{
	// デスクリプタヒープの設定
	ID3D12DescriptorHeap* descriptorHeaps[] = { DescriptorHeap.Get() };
	graphicsDevice->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);
}

void SrvManager::SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex)
{
	graphicsDevice->GetCommandList()->SetGraphicsRootDescriptorTable(
		RootParameterIndex,
		GetGPUDescriptorHandle(srvIndex)
	);
}

SrvManager* SrvManager::GetInstance() {
	static SrvManager instance;
	return &instance;
}

bool SrvManager::Check()
{
	return (useIndex < kMaxSRVCount);
}

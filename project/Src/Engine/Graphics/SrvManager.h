#pragma once
#include <cstdint>
#include <wrl.h>
#include <d3d12.h>

class GraphicsDevice;
class SrvManager
{
private:
	GraphicsDevice* graphicsDevice = nullptr;
	// 最大SRV数
	// デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap;
	// デスクリプタサイズ
	uint32_t descriptorSize = 0;
	// 次に使用するSRVのインデックス
	uint32_t useIndex = 0;
	// SRV生成 
	void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource,DXGI_FORMAT format,UINT MipLevels);
	void CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

	void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);

public:
	void Initialize(GraphicsDevice* graphicsDevice_);
	void PreDraw();
	uint32_t Allocate(); 
	static SrvManager* GetInstance();
	// 
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

	ID3D12DescriptorHeap* GetDescriptorHeap() { return DescriptorHeap.Get(); }
	static const uint32_t kMaxSRVCount;
	//　確保可能チェック
	bool Check();
};


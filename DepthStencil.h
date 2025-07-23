#pragma once
#include "Structs.h"

class DepthStencil
{
public:
	Microsoft::WRL::ComPtr<ID3D12Resource> Create(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height);


private:

	Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_{};
};


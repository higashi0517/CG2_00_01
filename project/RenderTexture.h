#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <cstdint> 
#include "Matrix4x4.h" 

using namespace Microsoft::WRL;

class RenderTexture
{
public:
    void Initialize(
        ComPtr<ID3D12Device> device,
        uint32_t width, uint32_t height,
        DXGI_FORMAT format, const Vector4& clearColor,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE srvGPUHandle
    );

    void TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList);
    void TransitionToShaderResource(ID3D12GraphicsCommandList* commandList);

    // ゲッター群
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtvCPUHandle() const { return rtvCPUHandle_; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSrvCPUHandle() const { return srvCPUHandle_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGPUHandle() const { return srvGPUHandle_; }
    ID3D12Resource* GetResource() const { return resource_.Get(); }

private:
    ComPtr<ID3D12Resource> CreateRenderTextureResource(
        ComPtr<ID3D12Device> device,
        uint32_t width, uint32_t height,
        DXGI_FORMAT format, const Vector4& clearColor
    );

    ComPtr<ID3D12Resource> resource_;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle_;
    D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle_;
    D3D12_GPU_DESCRIPTOR_HANDLE srvGPUHandle_;
    D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_GENERIC_READ;
};
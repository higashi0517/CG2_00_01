#include "RenderTexture.h"
#include <cassert>

using namespace Microsoft::WRL;

// ★ CreateRenderTextureResource のリソース生成部分の修正
ComPtr<ID3D12Resource> RenderTexture::CreateRenderTextureResource(
    ComPtr<ID3D12Device> device, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor)
{
    ComPtr<ID3D12Resource> resource = nullptr;

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 修正: BUFFERではなくTEXTURE2D
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Format = format;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = format;
    clearValue.Color[0] = clearColor.x;
    clearValue.Color[1] = clearColor.y;
    clearValue.Color[2] = clearColor.z;
    clearValue.Color[3] = clearColor.w;

    // &resource (ComPtrのポインタ) を渡す
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, &clearValue, IID_PPV_ARGS(&resource)
    );
    assert(SUCCEEDED(hr));

    return resource;
}

void RenderTexture::Initialize(
    ComPtr<ID3D12Device> device, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor,
    D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle, D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE srvGPUHandle)
{
    rtvCPUHandle_ = rtvCPUHandle;
    srvCPUHandle_ = srvCPUHandle;
    srvGPUHandle_ = srvGPUHandle;

    resource_ = CreateRenderTextureResource(device, width, height, format, clearColor);

    // RTVの生成
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    device->CreateRenderTargetView(resource_.Get(), &rtvDesc, rtvCPUHandle_);

    // SRVの生成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCPUHandle_);

    currentState_ = D3D12_RESOURCE_STATE_GENERIC_READ;
}

void RenderTexture::TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList)
{
    if (currentState_ == D3D12_RESOURCE_STATE_RENDER_TARGET) return;

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource_.Get();
    barrier.Transition.StateBefore = currentState_;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    currentState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
}

void RenderTexture::TransitionToShaderResource(ID3D12GraphicsCommandList* commandList)
{
    if (currentState_ == D3D12_RESOURCE_STATE_GENERIC_READ) return;

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource_.Get();
    barrier.Transition.StateBefore = currentState_;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    currentState_ = D3D12_RESOURCE_STATE_GENERIC_READ;
}


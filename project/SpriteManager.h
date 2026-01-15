#pragma once

#include <d3d12.h>
#include <wrl.h>      

class GraphicsDevice;

class SpriteManager {
public:
    void Initialize(GraphicsDevice* graphicsDevice);
    GraphicsDevice* GetGraphicsDevice() const { return graphicsDevice_; }
    void SetCommonRenderState();

private:
    void CreateRootSignature();
    void CreateGraphicsPipelineState();

private:
    GraphicsDevice* graphicsDevice_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
};

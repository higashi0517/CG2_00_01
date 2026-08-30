#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

class GraphicsDevice;
struct SkinCluster;


class SkinningCompute
{
public:
    void Initialize(
        GraphicsDevice* graphicsDevice
    );

    void Dispatch(
        SkinCluster& skinCluster,
        uint32_t numVertices
    );


private:
    void CreateRootSignature();
    void CreatePipelineState();

    GraphicsDevice* graphicsDevice_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>
        rootSignature_;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>
        pipelineState_;
};
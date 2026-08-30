#include "SkinningCompute.h"
#include "GraphicsDevice.h"
#include "Skeleton.h"

#include <cassert>
#include <cstdint>

void SkinningCompute::Initialize(
    GraphicsDevice* graphicsDevice)
{
    graphicsDevice_ = graphicsDevice;

    CreateRootSignature();
    CreatePipelineState();
}

void SkinningCompute::CreateRootSignature()
{
    D3D12_DESCRIPTOR_RANGE descriptorRanges[4]{};

    // MatrixPalette : t0
    descriptorRanges[0].RangeType =
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[0].NumDescriptors = 1;
    descriptorRanges[0].BaseShaderRegister = 0;
    descriptorRanges[0].OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // InputVertices : t1
    descriptorRanges[1].RangeType =
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[1].NumDescriptors = 1;
    descriptorRanges[1].BaseShaderRegister = 1;
    descriptorRanges[1].OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Influences : t2
    descriptorRanges[2].RangeType =
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[2].NumDescriptors = 1;
    descriptorRanges[2].BaseShaderRegister = 2;
    descriptorRanges[2].OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // OutputVertices : u0
    descriptorRanges[3].RangeType =
        D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptorRanges[3].NumDescriptors = 1;
    descriptorRanges[3].BaseShaderRegister = 0;
    descriptorRanges[3].OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[5]{};

    for (uint32_t index = 0; index < 4; ++index) {
        rootParameters[index].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        rootParameters[index].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_ALL;

        rootParameters[index]
            .DescriptorTable.pDescriptorRanges =
            &descriptorRanges[index];

        rootParameters[index]
            .DescriptorTable.NumDescriptorRanges = 1;
    }

    // SkinningInformation : b0
    rootParameters[4].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[4].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[4].Descriptor.ShaderRegister = 0;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumParameters =
        _countof(rootParameters);
    rootSignatureDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_NONE;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob
    );

    assert(SUCCEEDED(hr));

    hr = graphicsDevice_->GetDevice()
        ->CreateRootSignature(
            0,
            signatureBlob->GetBufferPointer(),
            signatureBlob->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature_)
        );

    assert(SUCCEEDED(hr));
}

void SkinningCompute::CreatePipelineState()
{
    auto computeShaderBlob =
        graphicsDevice_->CompileShader(
            L"Resources/shaders/Skinning.CS.hlsl",
            L"cs_6_0"
        );

    assert(computeShaderBlob != nullptr);

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};

    desc.pRootSignature =
        rootSignature_.Get();

    desc.CS.pShaderBytecode =
        computeShaderBlob->GetBufferPointer();

    desc.CS.BytecodeLength =
        computeShaderBlob->GetBufferSize();

    HRESULT hr =
        graphicsDevice_->GetDevice()
        ->CreateComputePipelineState(
            &desc,
            IID_PPV_ARGS(&pipelineState_)
        );

    assert(SUCCEEDED(hr));
}

void SkinningCompute::Dispatch(
    SkinCluster& skinCluster,
    uint32_t numVertices)
{
    if (numVertices == 0) {
        return;
    }

    auto commandList = graphicsDevice_->GetCommandList();

    commandList->SetComputeRootSignature(
        rootSignature_.Get()
    );

    commandList->SetPipelineState(
        pipelineState_.Get()
    );

    // t0 : MatrixPalette
    commandList->SetComputeRootDescriptorTable(
        0,
        skinCluster.paletteSrvHandle.second
    );

    // t1 : 元の頂点
    commandList->SetComputeRootDescriptorTable(
        1,
        skinCluster.inputVertexSrvHandle
    );

    // t2 : Influence
    commandList->SetComputeRootDescriptorTable(
        2,
        skinCluster.influenceSrvHandle
    );

    // u0 : スキニング後の頂点
    commandList->SetComputeRootDescriptorTable(
        3,
        skinCluster.outputVertexUavHandle
    );

    // b0 : 頂点数
    commandList->SetComputeRootConstantBufferView(
        4,
        skinCluster.skinningInformationResource
        ->GetGPUVirtualAddress()
    );

    // numthreads(1024, 1, 1)なので、
    // 頂点数を1024単位で切り上げる
    const UINT threadGroupCount =
        (numVertices + 1023) / 1024;

    commandList->Dispatch(
        threadGroupCount,
        1,
        1
    );
}
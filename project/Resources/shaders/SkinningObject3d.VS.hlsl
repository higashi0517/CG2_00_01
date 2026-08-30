#include "Object3d.hlsli"

struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix
    : register(b0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal   : NORMAL0;
    float32_t4 weight   : WEIGHT0;
    uint32_t4 index     : INDEX0;
};

struct Well
{
    float32_t4x4 skeletonSpaceMatrix;
    float32_t4x4 skeletonSpaceInverseTransposeMatrix;
};

StructuredBuffer<Well> gMatrixPalette
    : register(t0);

struct Skinned
{
    float32_t4 position;
    float32_t3 normal;
};

Skinned Skinning(VertexShaderInput input)
{
    Skinned skinned;

    // Compute Shaderですでに変形済み
    skinned.position = input.position;
    skinned.normal = input.normal;

    return skinned;
}

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;

    Skinned skinned = Skinning(input);

    output.position =
        mul(
            skinned.position,
            gTransformationMatrix.WVP
        );

    output.texcoord = input.texcoord;

    output.normal =
        normalize(
            mul(
                skinned.normal,
                (float32_t3x3)gTransformationMatrix.World
            )
        );

    return output;
}
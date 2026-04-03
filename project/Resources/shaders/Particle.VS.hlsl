#include "Particle.hlsli"

struct ParticleForGPU
{
    float32_t4x4 WVP;
    float32_t4x4 World;
    float32_t4 color;
};

StructuredBuffer<ParticleForGPU> gParticle : register(t0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float32_t intensity;
};

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceId : SV_InstanceID)
{
    VertexShaderOutput output;

    float32_t4x4 wvp = gParticle[instanceId].WVP;
    float32_t4x4 world = gParticle[instanceId].World;

    // 位置を WVP で変換
    output.position = mul(input.position, wvp);
    output.texcoord = input.texcoord;
    output.color = gParticle[instanceId].color;

    // 法線を World 行列で変換（平行移動は無視）
    //output.normal = normalize(mul(input.normal, (float32_t3x3)world));

    return output;
}
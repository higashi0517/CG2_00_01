#include "Particle.hlsli"

struct Material
{
    float32_t4 color;
    //int32_t enableLighting;
    //float32_t3 _padding; // C++ 側と同じ 16byte 揃え用
    float32_t4x4 uvTransform;
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float32_t intensity;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // UV 変換
    float32_t4 uv4 = float32_t4(input.texcoord, 0.0, 1.0);
    float32_t2 uv = mul(uv4, gMaterial.uvTransform).xy;

    float32_t4 textureColor = gTexture.Sample(gSampler, uv);

    float32_t4 resultColor = gMaterial.color * textureColor;

    // 簡単な平行光ライティング（必要なければ enableLighting=0 にする）
    //if (gMaterial.enableLighting != 0)
    //{
    //    float32_t3 n = normalize(input.normal);
    //    float32_t3 l = normalize(-gDirectionalLight.direction);
    //    float32_t cosTheta = saturate(dot(n, l));
    //    resultColor *= gDirectionalLight.color * (cosTheta * gDirectionalLight.intensity);
    //}

    output.color = resultColor;

    // アルファ0なら破棄
    if (output.color.a == 0.0)
    {
        discard;
    }

    return output;
}

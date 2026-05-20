#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
TextureCube<float32_t4> gEnvironmentMap : register(t1);

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct CameraData
{
    float3 worldPosition;
};

ConstantBuffer<CameraData> gCamera : register(b2);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    //float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    float4 transfomedUV = mul(float32_t4(input.texcoord,0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transfomedUV.xy);
    
    if (gMaterial.enableLighting != 0)
    {
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
       // float cos = saturate(dot(normalize(input.normal), -gDirectionalLight.direction));
        output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;
        
        float32_t3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
        float32_t3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
        float32_t4 environmentColor = gEnvironmentMap.Sample(gSampler, reflectedVector);
        
        output.color.rgb += environmentColor.rgb;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    
    return output;
}

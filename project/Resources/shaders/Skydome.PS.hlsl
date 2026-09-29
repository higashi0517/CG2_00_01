Texture2D<float4> skyTexture : register(t0);
SamplerState skySampler : register(s0);

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 main(PixelInput input) : SV_TARGET
{
    // Unlit: display the sky image without character lighting.
    return float4(skyTexture.Sample(skySampler, input.texcoord).rgb, 1.0f);
}

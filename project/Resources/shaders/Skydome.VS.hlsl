cbuffer CameraMatrix : register(b0)
{
    float4x4 viewProjection;
};

struct VertexInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    // Always place the sky at the far plane; the sphere follows the camera.
    output.position = mul(input.position, viewProjection).xyww;
    output.texcoord = input.texcoord;
    return output;
}

#include <metal_stdlib>
using namespace metal;

struct VSOut
{
    float4 position [[position]];
    float2 uv;
};



vertex VSOut vs(uint vertexID [[vertex_id]])
{
    constexpr float2 positions[3] =
    {
        float2(-1.0,  3.0),
        float2( 3.0, -1.0),
        float2(-1.0, -1.0)
    };
    

    VSOut out;

    out.position = float4(positions[vertexID], 0.0, 1.0);
    out.uv = (positions[vertexID] + 1.0) * 0.5;

    return out;
}


struct Camera {
    float4x4 invViewProj;
    float2 screenSize;
};
 fragment float4 fs(
    VSOut in [[stage_in]],
   constant Camera& camera [[buffer(2)]],
   texturecube<float> envTex [[texture(3)]],
   sampler envSampler [[sampler(0)]]
) {

float2 uv = in.uv;
float2 ndc = float2(uv.x * 2.0 - 1.0,  (uv.y * 2.0 - 1.0));
float4 clipDir = camera.invViewProj * float4(ndc, 1.0, 1.0);
float3 dir = normalize(clipDir.xyz / clipDir.w);
float3 color = envTex.sample(envSampler, dir).rgb;
return float4(color, 1.0);

}
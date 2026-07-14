#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

vertex VertexOut vs(
    uint vertexID [[vertex_id]]
) {
    constexpr float2 positions[3] =
    {
        float2(-1.0,  3.0),
        float2( 3.0, -1.0),
        float2(-1.0, -1.0)
    };

     VertexOut out;

    out.position = float4(positions[vertexID], 0.0, 1.0);
    out.uv = (positions[vertexID] + 1.0) * 0.5;

    return out;
}



fragment float4 fs(
    VertexOut in [[stage_in]],
    texture2d<float> finalTexture [[texture(2)]],
    sampler texSamp [[sampler(0)]]
) {
    float3 finalColor = finalTexture.sample(texSamp, in.uv).rgb;
    return float4(finalColor, 1.0);
}
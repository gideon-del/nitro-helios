#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float3 color;
};

struct PushConstant {
    float4x4 model;
};
vertex VertexOut vs(uint vid [[vertex_id]],
                    constant PushConstant& p [[buffer(1)]]
                    ) {
    float2 positions[3] = {
        float2( 0.0, -0.5),
        float2( 0.5,  0.5),
        float2(-0.5,  0.5)
    };
    float3 colors[3] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };
    VertexOut out;
    out.position = p.model * float4(positions[vid], 1.0, 1.0);
    out.color = colors[vid];
    return out;
}

fragment float4 fs(VertexOut in [[stage_in]]) {
    return float4(in.color, 1.0);
}

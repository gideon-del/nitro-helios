#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
};
struct VertexOut {
    float4 position [[position]];
    float3 color;
};

struct PushConstant {
    float4x4 model;
};
struct UniformBuffer {
    float4x4 view;
    float4x4 proj;
};
vertex VertexOut vs(VertexIn in [[stage_in]],
                    constant PushConstant& p [[buffer(1)]],
                      constant UniformBuffer& u [[buffer(2)]]
                    ) {
   
    VertexOut out;
    out.position = u.proj * u.view * p.model * float4(in.position, 1.0);
    out.color = in.color;
    return out;
}

fragment float4 fs(VertexOut in [[stage_in]]) {
    return float4(in.color, 1.0);
}

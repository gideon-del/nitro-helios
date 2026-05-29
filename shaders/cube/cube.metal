#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
    float3 normal [[attribute(2)]];
    float2 uv [[attribute(3)]];
};
struct VertexOut {
    float4 position [[position]];
    float3 color;
    float3 normal;
     float2 uv;
};

struct PushConstant {
    float4x4 model;
    float4x4 normalMatrix;
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
    float3x3 normalMatrix = {
        p.normalMatrix[0].xyz,
        p.normalMatrix[1].xyz,
        p.normalMatrix[2].xyz,
    };
    out.normal = normalMatrix * in.normal;
    out.uv = in.uv;
    return out;
}

fragment float4 fs(VertexOut in [[stage_in]]) {
    float3 N = normalize(in.normal);
    float3 L = normalize(float3(1.0,1.0,0.0));
    
    float intensity = max(0.0, dot(N,L));
    return float4(in.uv, 0.0, 1.0);
}

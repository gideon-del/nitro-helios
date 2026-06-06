#include <metal_stdlib>
using namespace metal;


struct VertexIn {
    float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
    float3 normal [[attribute(2)]];
    float2 uv [[attribute(3)]];
};


struct  LightTransform {
    float4x4 lightSpaceView[4];
};

struct PushConstant {
    float4x4 model;
    int cascadeIndex;
};

struct VertexOut  {
     float4 position [[position]];
};

vertex VertexOut vs(
    VertexIn in [[stage_in]],
     constant PushConstant& p [[buffer(1)]],
    constant LightTransform& lt [[buffer(2)]]
) {
VertexOut out;

out.position = lt.lightSpaceView[p.cascadeIndex] * p.model * float4(in.position, 1.0);
out.position.y = -out.position.y;
return out;
}
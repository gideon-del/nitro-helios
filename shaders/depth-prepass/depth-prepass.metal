#include <metal_stdlib>
using namespace metal;


struct VertexIn {
  float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
    float3 normal [[attribute(2)]];
    float2 uv [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]][[invariant]];
};

struct PushConstant {
    float4x4 model;
    float4x4 normal;
};

struct Camera {
    float4x4 view;
    float4x4 proj;
};


vertex VertexOut vs(
    VertexIn in [[stage_in]],
    constant PushConstant& pc [[buffer(1)]],
    constant Camera& camera [[buffer(2)]]
) {
    VertexOut out;
   
   out.position = camera.proj * camera.view * pc.model * float4(in.position, 1.0);
    return out;
}

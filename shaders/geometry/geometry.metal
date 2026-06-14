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
    float3 normal;
    float2 uv;

};

struct PushConstant {
    float4x4 model;
    float4x4 normalMatrix;
};

struct FrameUniformBuffer {
    float4x4 view;
    float4x4 proj;
};   

struct GeometryBuffer {
    float4 albedo [[color(0)]];
    float4 normal [[color(1)]];
    float4 material [[color(2)]];
    float4 emissive [[color(3)]];
};
vertex VertexOut vs(
 VertexIn in [[stage_in]],
    constant PushConstant& p [[buffer(1)]],
    constant FrameUniformBuffer& fub [[buffer(2)]]   
){
     VertexOut out;
    out.position = fub.proj * fub.view * p.model * float4(in.position, 1.0); 
    float3x3 normalMatrix = {
        p.normalMatrix[0].xyz,
        p.normalMatrix[1].xyz,
        p.normalMatrix[2].xyz,
    };
    out.normal = normalMatrix * in.normal;
    out.uv = in.uv;

    return out;
}


float2 encodeNormal(float3 n) {
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    if (n.z < 0.0)
        return (1.0 - abs(n.yx)) * sign(n.xy) * 0.5 + 0.5;
    return n.xy * 0.5 + 0.5;
}



fragment GeometryBuffer fs(
    VertexOut in [[stage_in]]
) {
    GeometryBuffer out;
    out.normal = float4(encodeNormal(in.normal),0.0,1.0);
    out.albedo = float4(0.4,0.5,0.9,1.0);
    out.material =float4(1.0,1.0,1.0,1.0);
    out.emissive = float4(1.0,1.0,1.0,1.0);

    return out;
}

 
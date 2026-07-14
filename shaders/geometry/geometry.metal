#include <metal_stdlib>
using namespace metal;


struct VertexIn {
    float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
    float3 normal [[attribute(2)]];
    float2 uv [[attribute(3)]];
    float4 tangent [[attribute(4)]];
};

struct VertexOut {
    float4 position [[position]][[invariant]];
    float3 normal;
    float2 uv;
    float4 tangent;

};

struct PushConstant {
    float4x4 model;
    float4x4 normalMatrix;
    float4 baseColor;
    float metallic;
    float roughness;
    uint useTextures;
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

    out.tangent = float4(normalMatrix * in.tangent.xyz, in.tangent.w);
    return out;
}


float2 encodeNormal(float3 n) {
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    if (n.z < 0.0)
        return (1.0 - abs(n.yx)) * sign(n.xy) * 0.5 + 0.5;
    return n.xy * 0.5 + 0.5;
}



fragment GeometryBuffer fs(
    VertexOut in [[stage_in]],
    constant PushConstant& pc [[buffer(1)]],
    texture2d<float> baseColorTex [[texture(16)]],
    texture2d<float> normalTex [[texture(17)]],
    texture2d<float> metallicRoughnessTex [[texture(18)]],
    texture2d<float> aoTex [[texture(19)]],
    texture2d<float> emissiveTex [[texture(20)]],
    sampler materialSampler [[sampler(1)]]
) {
    GeometryBuffer out;
    float3 N = normalize(in.normal);
    float3 T = normalize(in.tangent.xyz);
    T = normalize(T - dot(N,T) * N);
float3 B = normalize(cross(N,T) * in.tangent.w);
 float3x3 TBN = float3x3(T,B,N);
    if(pc.useTextures == 1) {
    float4 metallicRoughness = metallicRoughnessTex.sample(materialSampler, in.uv);
    float ao = aoTex.sample(materialSampler, in.uv).r;
    float3 tangentNormal = normalTex.sample(materialSampler, in.uv).rgb;
    float3 worldNormal = normalize(TBN * tangentNormal);
    float3 emission = emissiveTex.sample(materialSampler, in.uv).rgb;
    float3 baseColor = baseColorTex.sample(materialSampler, in.uv).rgb;

     out.albedo = float4(baseColor,1.0);
     out.material = float4(ao, metallicRoughness.b, metallicRoughness.g,1.0);
     out.normal = float4(encodeNormal(worldNormal),0.0,1.0);
     out.emissive = float4(emission,1.0);
    }else {
    out.normal = float4(encodeNormal(in.normal),0.0,1.0);
    out.albedo = float4(pc.baseColor.rbg,1.0);
    out.material =float4(1.0,pc.metallic,pc.roughness,1.0);
    out.emissive = float4(1.0,1.0,1.0,1.0);
    }

    return out;
}

 
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
};

struct PushConstant {
    float4x4 model;
    float4 lightPosition;
    float4 lightColor;
    float2 screenSize;
    float radius;
    float intensity;
};


struct CameraVP {
   float4x4 view;
   float4x4 proj;
   float4x4 invViewProj;
};

struct VertexOut {
    float4 position [[position]];
};

vertex VertexOut vs(
    VertexIn in [[stage_in]],
    constant PushConstant& pc [[buffer(1)]],
    constant CameraVP& camera [[buffer(2)]]
) {
    VertexOut out;
    out.position = camera.proj * camera.view * pc.model * float4(in.position,1.0);
    return out;
}
float3 reconstructPosition(float2 uv, float depth, float4x4 invViewProj) {  
float4 clipPos =
    float4(
        uv.x * 2.0 - 1.0,
        (1.0 - uv.y) * 2.0 - 1.0,
        depth,
        1.0);
 float4 worldPos = invViewProj * clipPos;
 return worldPos.xyz / worldPos.w;
};

float3 decodeNormal(float2 n) {
   float2  f = n * 2.0 - 1.0;
   float3 v = float3(f, 1.0 - abs(f.x) - abs(f.y));

   if(v.z < 0) {
    v.xy =( 1.0 - abs(v.yx)) * sign(v.xy);
   }
   return normalize(v);
} 

fragment float4 fs(
    VertexOut in [[stage_in]],
     constant PushConstant& pc [[buffer(1)]],
    constant CameraVP& camera [[buffer(2)]],
    texture2d<float> gDepthTex [[texture(3)]],
    texture2d<float> gNormalTex [[texture(4)]],
   sampler gSamp [[sampler(0)]]
) {
float2 uv = in.position.xy / pc.screenSize;
float depth = gDepthTex.sample(gSamp, uv).r;
 float3 N = decodeNormal(gNormalTex.sample(gSamp,uv).rg);
  float3 worldPos = reconstructPosition(uv, depth, camera.invViewProj);

float3 PL = pc.lightPosition.xyz - worldPos;
float dist = length(PL);
PL = normalize(PL);  
 float attenuation = pow(max(0.0, 1.0 - pow(dist/pc.radius, 4)), 2)
   / (dist * dist);
float diffuse = max(0.0,dot(N,PL));


float3 finalColor = pc.lightColor.xyz * diffuse * attenuation * pc.intensity;

return float4(finalColor,1.0);

}
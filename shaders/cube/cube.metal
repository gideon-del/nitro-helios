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
     float4 fragLightPos;
     float3 fragPos;
};

struct PushConstant {
    float4x4 model;
    float4x4 normalMatrix;
};
struct FrameUniformBuffer {
    float4x4 view;
    float4x4 proj;

    float4 cameraPos;
    
    float4 lightPos;
    float4 lightColor;

    float4x4 lightViewProj;

    float ambient;
    float Ka;
    float Kd;
    float Ks;
    float shininess;
};
vertex VertexOut vs(VertexIn in [[stage_in]],
                    constant PushConstant& p [[buffer(1)]],
                      constant FrameUniformBuffer& fub [[buffer(2)]]
                    ) {
   
    VertexOut out;
    float4 worldPos =  p.model * float4(in.position, 1.0);
    out.position = fub.proj * fub.view * worldPos;
    out.color = in.color;
    float3x3 normalMatrix = {
        p.normalMatrix[0].xyz,
        p.normalMatrix[1].xyz,
        p.normalMatrix[2].xyz,
    };
    out.normal = normalMatrix * in.normal;
    out.uv = in.uv;
    out.fragLightPos = fub.lightViewProj * worldPos;
    out.fragPos = worldPos.xyz;
    return out;
}

fragment float4 fs(VertexOut in [[stage_in]],
  constant FrameUniformBuffer& fub [[buffer(2)]],
  texture2d<float> depthTex [[texture(0)]],
  sampler samp [[sampler(0)]]
) {
    float3 N = normalize(in.normal);
    float3 L = normalize(fub.lightPos.xyz - in.fragPos);
    float3 V = normalize(fub.cameraPos.xyz - in.fragPos);
    float3 H = normalize(L + V);

    float3 lightColor = fub.lightColor.xyz;
    float diffuse = max(0.0, dot(N,L));
    float specular = pow(max(0.0,dot(N,H) ), fub.shininess);
    float3 ambientColor = lightColor * fub.ambient * fub.Ka;
    float3 diffuseColor = lightColor * diffuse * fub.Kd;
    float3 specularColor = lightColor * specular * fub.Ks;
   
   float3 projCoords = in.fragLightPos.xyz / in.fragLightPos.w;

   projCoords.xy = projCoords.xy * 0.5 + 0.5;
   
   float closestDepth = depthTex.sample(samp,projCoords.xy).x;
   float currentDepth = projCoords.z;

   float bias = max(0.005 * (1.0 - dot(N,L)) , 0.0005); 

  float shadow = currentDepth-bias > closestDepth ? 0.0: 1.0;
  float3 color = float3(0.4,0.5,0.9);
 float3 finalColor = (ambientColor + shadow * (diffuseColor + specularColor)) * color; 

return float4(finalColor, 1.0);
}

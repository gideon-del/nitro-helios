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

float kernel_shadow(  depth2d<float> shadowMap, sampler samp,float3 projCoords, float bias) {
 float2 texelSize = 1.0 / float2(
    shadowMap.get_width(),
    shadowMap.get_height()
);

   float shadow = 0.0;


   for(int y=-1; y <= 1; y++) {
     for(int x=-1; x <= 1; x++) {     
     float2 offset = float2(x, y) * texelSize;
      shadow += shadowMap.sample_compare(samp, projCoords.xy + offset, projCoords.z - bias);
   }
   }
   
   return shadow / 9.0;
}
constant float2 poissonDisk[16] = {
   float2(-0.94201624, -0.39906216),
   float2( 0.94558609, -0.76890725),
   float2(-0.094184101,-0.92938870),
   float2( 0.34495938,  0.29387760),
   float2(-0.91588581,  0.45771432),
   float2(-0.81544232, -0.87912464),
   float2(-0.38277543,  0.27676845),
   float2( 0.97484398,  0.75648379),
   float2( 0.44323325, -0.97511554),
   float2( 0.53742981, -0.47373420),
   float2(-0.26496911, -0.41893023),
   float2( 0.79197514,  0.19090188),
   float2(-0.24188840,  0.99706507),
   float2(-0.81409955,  0.91437590),
   float2( 0.19984126,  0.78641367),
   float2( 0.14383161, -0.14100790)
};

float shadowPoisson(depth2d<float> shadowMap, sampler samp,float3 projCoords, float bias) {
  
  float2 texelSize = 1.0 / float2(
    shadowMap.get_width(),
    shadowMap.get_height()
);

   float shadow = 0.0;
   float radius = 2.0;
   for(int i =0; i < 16; i++) {
     shadow += shadowMap.sample_compare(samp,projCoords.xy + poissonDisk[i] * texelSize *radius, projCoords.z - bias);
   }

   return shadow / 16.0;
}

fragment float4 fs(VertexOut in [[stage_in]],
  constant FrameUniformBuffer& fub [[buffer(2)]],
  depth2d<float> depthTex [[texture(0)]],
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

float bias = max(
    0.005 * (1.0 - dot(N, L)),
    0.0005
);

  float shadow = shadowPoisson(depthTex,samp,projCoords,bias);
  float3 color = float3(0.4,0.5,0.9);
 float3 finalColor = (ambientColor + shadow * (diffuseColor + specularColor)) * color; 
return float4(finalColor, 1.0);
}

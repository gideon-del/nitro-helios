#include <metal_stdlib>
using namespace metal;

struct PointLight {
  float4 position;
  float4 color;
  float radius;
  float intensity;
};
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
    float4x4 lightViewProj[4];
    float4 cascadeSplit;
    float ambient;
    float Ka;
    float Kd;
    float Ks;
    float shininess;
    float shadowBias;
    float shadowNormalBias;
    float showCascadeColors;
    float debugMode;
    PointLight pointLights[1000];
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
    out.fragPos = worldPos.xyz;
    return out;
}

float kernel_shadow( depth2d<float> shadowMap, sampler samp,float3 projCoords, float bias) {
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

float shadowPoisson(depth2d<float> shadowMap, sampler samp,float4 fragLightPos, float bias) {
  
  float3 projCoords = fragLightPos.xyz /fragLightPos.w;
  projCoords.xy = projCoords.xy * 0.5 + 0.5;
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
float blendCascade( 
  float shadow0, 
  float shadow1,
  float split,
  float blendWidth, 
  float viewDepth
  ) {
float blend = smoothstep(
  split - blendWidth,
  split+ blendWidth,
  viewDepth
);

if(viewDepth < split-blendWidth) {
  return shadow0;
}else if(viewDepth <= split+blendWidth) {
return mix(shadow0,shadow1, blend);
}else {
  return shadow1;
}
}

float3 calculatePointLightColor(float3 worldPos, float3 albedo, constant FrameUniformBuffer& fub, float3 normal) {
float3 PLColor = float3(0.0);

for(int i =0; i <1000; i++) {
  float3 P = fub.pointLights[i].position.xyz - worldPos;
  float dist = length(P);
  if(dist > fub.pointLights[i].radius ) {
    continue;
  }
  P = normalize(P);
  float diffuse = max(0.0, dot(normal, P));
  float attenuation = pow(max(0.0, 1.0 - pow(dist/fub.pointLights[i].radius, 4)), 2)
   / (dist * dist);

 PLColor +=   albedo * fub.pointLights[i].color.xyz * diffuse *attenuation * fub.pointLights[i].intensity;
  
}

return PLColor;
};

fragment float4 fs(VertexOut in [[stage_in]],
  constant FrameUniformBuffer& fub [[buffer(2)]],
  depth2d<float> depthTex0 [[texture(16)]],
  depth2d<float> depthTex1 [[texture(17)]],
  depth2d<float> depthTex2 [[texture(18)]],
  depth2d<float> depthTex3 [[texture(19)]],
  sampler depthTexSamp [[sampler(1)]]
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
   
float bias = max(
   fub.shadowBias * (1.0 - dot(N, L)),
    0.0005
);
float normalBias = (1.0 - dot(N,L)) * fub.shadowNormalBias;
float3 shadowPos = in.fragPos + N * normalBias;

float4 viewPos = fub.view * float4(in.fragPos,1.0);
float viewDepth = -viewPos.z;

float blendWidth0 = (fub.cascadeSplit[1] - fub.cascadeSplit[0] ) * 0.15;
float blendWidth1 = (fub.cascadeSplit[2] - fub.cascadeSplit[1] ) * 0.15;
float blendWidth2 = (fub.cascadeSplit[3] - fub.cascadeSplit[2] ) * 0.15;
  float shadow = 0.0;
  float3 cascadeColor;
  if(viewDepth <= fub.cascadeSplit[0]+blendWidth0) {
  float  shadow0 = shadowPoisson(depthTex0,depthTexSamp,fub.lightViewProj[0] * float4(shadowPos, 1.0),bias);
float   shadow1 = shadowPoisson(depthTex1,depthTexSamp,fub.lightViewProj[1] * float4(shadowPos, 1.0),bias);
    shadow = blendCascade(shadow0, shadow1, fub.cascadeSplit[0],blendWidth0,viewDepth);
    cascadeColor = float3(1,0,0);
  }else if (viewDepth <= fub.cascadeSplit[1]+blendWidth1) {
float    shadow1 = shadowPoisson(depthTex1,depthTexSamp,fub.lightViewProj[1] * float4(shadowPos, 1.0),bias);
float   shadow2 = shadowPoisson(depthTex2,depthTexSamp,fub.lightViewProj[2] * float4(shadowPos, 1.0),bias);
    shadow = blendCascade(shadow1, shadow2, fub.cascadeSplit[1],blendWidth1,viewDepth);
      cascadeColor = float3(0,1,0);
  } else if (viewDepth < fub.cascadeSplit[2]+blendWidth2) {
   float shadow2 = shadowPoisson(depthTex2,depthTexSamp,fub.lightViewProj[2] * float4(shadowPos, 1.0),bias);
    float shadow3 = shadowPoisson(depthTex3,depthTexSamp,fub.lightViewProj[3] * float4(shadowPos, 1.0),bias);
    shadow = blendCascade(shadow2, shadow3, fub.cascadeSplit[2],blendWidth2,viewDepth);
      cascadeColor = float3(0,0,1);
  }else {
     shadow = shadowPoisson(depthTex3,depthTexSamp,fub.lightViewProj[3] * float4(shadowPos, 1.0),bias);
       cascadeColor = float3(1,1,0);
  }
  float3 color = float3(0.4,0.5,0.9);
 float3 directionalLightColor = (ambientColor +  shadow*(diffuseColor + specularColor)) * color; 
 float3 pointLightColor = calculatePointLightColor(in.fragPos, color,fub,N);
 float3 finalColor;
switch(int(fub.debugMode)) {
case 1: 
  finalColor = color;
  break;
case 2:
   finalColor = N;
   break;
case 3:
   finalColor =float3(in.fragPos.z * 0.05);
   break;
case 4:
   finalColor =in.fragPos * 0.05;
   break;
case 5:
   finalColor =cascadeColor;
   break;
case 6:
   finalColor =pointLightColor;
   break;
case 7:
   finalColor =directionalLightColor;
   break;
default:
  finalColor = directionalLightColor + pointLightColor;
  break;   
}
return float4(
finalColor,
    1.0);
}

#include <metal_stdlib>
using namespace metal;




struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

struct FrameUniformBuffer {
    float4 cameraPosition;
    float4 lightPosition;
    float4 lightColor;  
    float4x4 invViewProj;
    float4x4 view;
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
    float lightMode; 
    float roughness;   
};

vertex VertexOut vs(
    uint vid [[vertex_id]]
) {
    float2 positions[3] = {   
    
    float2(-1,-1),
     float2(3,-1),
     float2(-1,3)
} ;
    VertexOut out;
    out.position = float4(positions[vid], 0.0,1.0);
    out.uv =( positions[vid] + 1.0) * 0.5;
    out.uv.y = 1.0 -out.uv.y;
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

constant float PI = 3.14159265358979323846;

float3 blinnPhongShade(float3 worldPos, float3 normal, float shadow, FrameUniformBuffer fub) {

    float3 L = normalize(fub.lightPosition.xyz - worldPos);
    float3 V = normalize(fub.cameraPosition.xyz - worldPos);
    float3 H = normalize(L + V);

    float3 lightColor = fub.lightColor.xyz;
    float diffuse = max(0.0, dot(normal,L));
    float specular = pow(max(0.0,dot(normal,H) ), fub.shininess);
    float3 ambientColor = lightColor * fub.ambient * fub.Ka;
    float3 diffuseColor = lightColor * diffuse * fub.Kd;
    float3 specularColor = lightColor * specular * fub.Ks;
   
 return  (ambientColor +  shadow*(diffuseColor + specularColor));
    
}

struct CascadeResult {
  float shadow;
  float3 cascadeColor;
};

CascadeResult getCascadeShadow(float3 worldPos, float3 normal, FrameUniformBuffer fub,
depth2d<float> depthTex0,
depth2d<float> depthTex1, 
depth2d<float> depthTex2, 
depth2d<float> depthTex3, 
sampler depthTexSamp) {
  float3 L = normalize(fub.lightPosition.xyz - worldPos);
    float bias = max(
   fub.shadowBias * (1.0 - dot(normal, L)),
    0.0005
);
float normalBias = (1.0 - dot(normal,L)) * fub.shadowNormalBias;
float3 shadowPos =worldPos + normal * normalBias;

float4 viewPos = fub.view * float4(worldPos,1.0);
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

  CascadeResult result;
  result.shadow = shadow;
  result.cascadeColor =cascadeColor;
  return result;
};
float3 mapToHeatColor(float value, float minVal, float maxVal, uint ramp) {
float t = clamp((value-minVal)/(maxVal-minVal),0.0, 1.0);
if(ramp == 0) return float3(t);
if(ramp == 1) return mix(float3(0.2,0.0,0.4), float3(1.0,0.9,0.2), t);
return mix(float3(0,0,1), float3(1,0,0), t);
}


float3 lambertDiffuse(float3 albedo, float3 N, float3 L) {
  float3 NdotL = max(dot(N,L),0.0);
  return albedo / PI * NdotL;
}

float distributionGGX(float3 N, float3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a*a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;
  return a2/max(denom, 0.00001);
}

float3 fresnelSchlick(float3 F0, float cosTheta) {
  return F0 + (1.0 - F0)*pow(clamp(1.0- cosTheta, 0.0,1.0),5);
}

float geometrySchlickGGX(float NdotV, float roughness) {
  float r = roughness +1.0;
  float k = (r*r)/8.0;
  return NdotV/(NdotV * (1.0-k) + k);
}

float geometrySmith(float3 N, float3 L, float3 V, float roughness) {
  float ggx_L = geometrySchlickGGX(max(dot(N,L), 0.0), roughness);
  float ggx_V = geometrySchlickGGX(max(dot(N,V), 0.0), roughness);
  return ggx_V * ggx_L;
}

float3 cookTorranceSpecular(float3 N, float3 V, float3 L, float3 H,
                           float roughness, float3 F0) {
 float D = distributionGGX(N, H, roughness);
 float3 F = fresnelSchlick(F0,max(dot(H,V),0.0));
 float G = geometrySmith(N,L,V,roughness);
 float NdotL = max(dot(N,L),0.0);
 float NdotV = max(dot(N,V),0.0);
 float denom = 4.0 * NdotL * NdotV + 0.00001;

 return (D*F*G)/ denom;
}

float3 metallicDiffuse(float3 F ,float metallic) {
  float3 kS = F;
  float3 kD = float3(1.0) - kS;
return kD * (1.0 - metallic);
}


float3 sampleEnvironment(float3 N, float3 V, texturecube<float> envTexture, sampler envSampler) {
  float3 R = reflect(-V,N);

 return envTexture.sample(envSampler,R).rgb;
}

float3 diffuseIBL(float3 N, float3 albedo, float metallic, texturecube<float> irradianceMap, sampler irradianceSampler) {
    float3 irradiance = irradianceMap.sample(irradianceSampler, N).rgb;

    float3 kD = albedo * (1.0 - metallic);

    return kD * irradiance;
}

float3 specularIBL(float3 N, float3 V, float roughness, float3 F0,texturecube<float>  prefilterEnv, sampler prefilterSampler, texture2d<float> brdfLUTTex, sampler brdfSampler) {
float3 R = reflect(-V, N);
float maxMip = 4.0; 
 float3 prefilteredColor = prefilterEnv.sample(prefilterSampler, R, level(roughness * maxMip)).rgb;

 float NdotV = max(dot(N, V), 0.0);
  float2 envBRDF = brdfLUTTex.sample(brdfSampler, float2(NdotV, roughness)).rg;

 return prefilteredColor * (F0 * envBRDF.x + envBRDF.y); 

}


fragment float4 fs(
    VertexOut in [[stage_in]],
  constant FrameUniformBuffer& fub [[buffer(2)]],

   texture2d<float> gAlbedoTex [[texture(16)]],    
   texture2d<float> gNormalTex [[texture(17)]], 
   texture2d<float> gMaterialTex [[texture(18)]], 
   texture2d<float> gEmissiveTex [[texture(19)]], 
   texture2d<float> gDepthTex [[texture(20)]], 
   texture2d<float> lightingTex [[texture(21)]], 
   texturecube<float> irradianceTex [[texture(22)]], 
   texture2d<float> environment [[texture(23)]], 
   texturecube<float> prefilterMap [[texture(24)]], 
   texture2d<float> brdfLUT [[texture(25)]], 
    sampler gSamp [[sampler(1)]],

   depth2d<float> depthTex0 [[texture(32)]],
  depth2d<float> depthTex1 [[texture(33)]],
  depth2d<float> depthTex2 [[texture(34)]],
  depth2d<float> depthTex3 [[texture(35)]],
  sampler depthTexSamp [[sampler(2)]]
) {
  float depth = gDepthTex.sample(gSamp, in.uv).r;
  if(depth >= 1.0) {
   
   float3 color = environment.sample(gSamp, in.uv).rgb;
   return float4(color, 1.0);
  }
  float3 albedo = gAlbedoTex.sample(gSamp, in.uv).rgb;
  float3 N = decodeNormal(gNormalTex.sample(gSamp,in.uv).rg);
   float3 metallicRoughness = gMaterialTex.sample(gSamp, in.uv).rgb;
   float metallic = metallicRoughness.g;
   float roughness = metallicRoughness.b;
   float ao = metallicRoughness.r;
  float3 worldPos = reconstructPosition(in.uv, depth, fub.invViewProj);
 float3 finalColor;
 CascadeResult cascadeResult = getCascadeShadow(worldPos, N, fub, depthTex0, depthTex1,depthTex2, depthTex3, depthTexSamp);
 float shadow = cascadeResult.shadow;
 float3 cascadeColor = cascadeResult.cascadeColor;

 float3 pointLightColor = lightingTex.sample(gSamp, in.uv).rgb ;
 float3 L =  normalize(fub.lightPosition.xyz - worldPos);
float3 V = normalize(fub.cameraPosition.xyz - worldPos);
float3 H = normalize(L + V);
 float3 dielectricF0 = float3(0.04);
 float3 F0 = mix(dielectricF0, albedo,metallic);
float D = distributionGGX(N, H,roughness);
float3 F = fresnelSchlick(F0, max(dot(N,V),0.0));
float G = geometrySmith(N, L, V,roughness);

float3 directDiffuse = metallicDiffuse(F, metallic) * lambertDiffuse(albedo,N,L);
float3 directSpecular = cookTorranceSpecular(N,L,V,H,roughness,F0);

float3 ambient = diffuseIBL(N, albedo, metallic, irradianceTex, gSamp) * ao;
float3 specularIBLColor = specularIBL(N, V, roughness, F0, prefilterMap, gSamp, brdfLUT, gSamp) * ao;

float3 directionalLighting;

switch(int(fub.lightMode)){
  case 0:
   directionalLighting = blinnPhongShade(worldPos, N, shadow, fub);
   break;
  case 1:
   directionalLighting =  lambertDiffuse(albedo, N,L);
   break;
   default:
    directionalLighting = directDiffuse + directSpecular + ambient + specularIBLColor;
}
switch(int(fub.debugMode)) {
case 1: 
  finalColor = albedo;
  break;
case 2:
   finalColor = N;
   break;
case 3:
   finalColor =float3(worldPos.z * 0.05);
   break;
case 4:
   finalColor =worldPos * 0.05;
   break;
case 5:
   finalColor =cascadeColor;
   break;
case 6:
   finalColor =pointLightColor;
   break;
case 7:
   finalColor =directionalLighting;
   break;
case 8:
   finalColor =pointLightColor;
   break;
 case 9:
    finalColor = mapToHeatColor(D,0.0,1.0,2);
    break;
  case 10:
    finalColor = F;
    break;
  case 11:
    finalColor = mapToHeatColor(G, 0.0,1.0,0);
    break;
  case 12:
    finalColor = ambient;
    break;
  case 13:
    finalColor = specularIBLColor;
    break;  
default:
  finalColor = (directionalLighting+ pointLightColor) * albedo;
  break;   
}

 return float4(
finalColor,
    1.0);
}


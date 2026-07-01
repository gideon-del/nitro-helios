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




fragment float4 fs(
    VertexOut in [[stage_in]],
  constant FrameUniformBuffer& fub [[buffer(2)]],

   texture2d<float> gAlbedoTex [[texture(16)]],    
   texture2d<float> gNormalTex [[texture(17)]], 
   texture2d<float> gMaterialTex [[texture(18)]], 
   texture2d<float> gEmissiveTex [[texture(19)]], 
   texture2d<float> gDepthTex [[texture(20)]], 
   texture2d<float> lightingTex [[texture(21)]], 
    sampler gSamp [[sampler(1)]],

   depth2d<float> depthTex0 [[texture(32)]],
  depth2d<float> depthTex1 [[texture(33)]],
  depth2d<float> depthTex2 [[texture(34)]],
  depth2d<float> depthTex3 [[texture(35)]],
  sampler depthTexSamp [[sampler(2)]]
) {
  float depth = gDepthTex.sample(gSamp, in.uv).r;
  if(depth >= 1.0) {
discard_fragment();
  }
  float3 albedo = gAlbedoTex.sample(gSamp, in.uv).rgb;
  float3 N = decodeNormal(gNormalTex.sample(gSamp,in.uv).rg);
  float3 worldPos = reconstructPosition(in.uv, depth, fub.invViewProj);


   float3 L = normalize(fub.lightPosition.xyz - worldPos);
    float3 V = normalize(fub.cameraPosition.xyz - worldPos);
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
float3 shadowPos =worldPos + N * normalBias;

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
float3 finalColor;
 float3 directionalLightColor = (ambientColor +  shadow*(diffuseColor + specularColor)) * albedo; 
 float3 pointLightColor = lightingTex.sample(gSamp, in.uv).rgb * albedo;
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
   finalColor =directionalLightColor;
   break;
case 8:
   finalColor =pointLightColor;
   break;
default:
  finalColor = directionalLightColor + pointLightColor;
  break;   
}

 return float4(
finalColor,
    1.0);
}


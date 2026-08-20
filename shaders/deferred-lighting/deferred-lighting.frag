#version 450



layout(set=0, binding=2)uniform  FrameUniformBuffer { 
    vec4 cameraPosition;
    vec4 lightPosition;
    vec4 lightColor;
    
    mat4 invViewProj;
    mat4 view;
    mat4 lightViewProj[4];
    vec4 cascadeSplit; 
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
} frameUbo;


layout(set=1, binding=0) uniform sampler2D gAlbedo;
layout(set=1, binding=1) uniform sampler2D gNormal;
layout(set=1, binding=2) uniform sampler2D gMaterial;
layout(set=1, binding=3) uniform sampler2D gEmissive;
layout(set=1, binding=4) uniform sampler2D gDepth;
layout(set=1, binding=5) uniform sampler2D lightShading;
layout(set=1, binding=6) uniform samplerCube irradianceTex;
layout(set=1, binding=7) uniform sampler2D environment;
layout(set=1, binding=8) uniform samplerCube prefilterMap;
layout(set=1, binding=9) uniform sampler2D brdfLUT;
layout(set=1, binding=10) uniform sampler2D ssaoTexture;

layout(set=2, binding=0) uniform sampler2DShadow shadowMap0;
layout(set=2, binding=1) uniform sampler2DShadow shadowMap1;
layout(set=2, binding=2) uniform sampler2DShadow shadowMap2;
layout(set=2, binding=3) uniform sampler2DShadow shadowMap3;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2  fragUV;

const float PI = 3.14159265358979323846;
const float TWO_PI = 6.28318530717958647692;
const float HALF_PI = 1.57079632679489661923;

vec3 reconstructPosition(vec2 uv, float depth, mat4 invViewProj) {
 vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
 vec4 worldPos = invViewProj * clipPos;
 return worldPos.xyz /worldPos.w;
}




vec3 decodeNormal(vec2 n) {
vec2 f = n * 2.0 - 1.0;
vec3 v = vec3(
    f.x,
    f.y,
    1.0 - abs(f.x) - abs(f.y));

if (v.z < 0.0)
{
    v.xy =
        (1.0 - abs(v.yx))
        * sign(v.xy);
}

return normalize(v);
}


vec2 poissonDisk[16] = vec2[](
   vec2(-0.94201624, -0.39906216),
   vec2( 0.94558609, -0.76890725),
   vec2(-0.094184101,-0.92938870),
   vec2( 0.34495938,  0.29387760),
   vec2(-0.91588581,  0.45771432),
   vec2(-0.81544232, -0.87912464),
   vec2(-0.38277543,  0.27676845),
   vec2( 0.97484398,  0.75648379),
   vec2( 0.44323325, -0.97511554),
   vec2( 0.53742981, -0.47373420),
   vec2(-0.26496911, -0.41893023),
   vec2( 0.79197514,  0.19090188),
   vec2(-0.24188840,  0.99706507),
   vec2(-0.81409955,  0.91437590),
   vec2( 0.19984126,  0.78641367),
   vec2( 0.14383161, -0.14100790)
);

float shadowPoisson(vec4 fragLightPos, float bias, sampler2DShadow shadowMap) {
     vec3 projCoords = fragLightPos.xyz / fragLightPos.w;
   projCoords.xy = projCoords.xy * 0.5 + 0.5;
  
   if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
    projCoords.y < 0.0 || projCoords.y > 1.0 ||
    projCoords.z < 0.0 || projCoords.z > 1.0)
{
    return 1.0; 
}
   vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

   float shadow = 0.0;
   float radius = 2.0;
   for(int i =0; i < 16; i++) {
     
     shadow += texture(shadowMap,vec3(projCoords.xy + poissonDisk[i] * texelSize *radius,projCoords.z - bias));
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

struct CascadeResult {
  float shadow;
  vec3 cascadeColor;
};
CascadeResult getCascadeShadow(
vec3 worldPos, 
vec3 normal) {
  vec3 L = normalize(frameUbo.lightPosition.xyz - worldPos);
 float bias = max(
   frameUbo.shadowBias * (1.0 - dot(normal, L)),
    0.0005
   );
  

  float normalBias =
      (1.0 - dot(normal, L)) * frameUbo.shadowNormalBias;

  vec3 shadowPos =
      worldPos + normal * normalBias;

  float shadow = 0.0;   
  vec4 viewPos =
      frameUbo.view * vec4(worldPos, 1.0);

  float viewDepth = -viewPos.z;
  vec3 cascadeColor;
  float blendWidth0 = (frameUbo.cascadeSplit[1] - frameUbo.cascadeSplit[0] ) * 0.15;
  float blendWidth1 = (frameUbo.cascadeSplit[2] - frameUbo.cascadeSplit[1] ) * 0.15;
  float blendWidth2 = (frameUbo.cascadeSplit[3] - frameUbo.cascadeSplit[2] ) * 0.15; 

 if( viewDepth <= frameUbo.cascadeSplit[0]+blendWidth0) {
   float shadow0 =shadowPoisson(frameUbo.lightViewProj[0] * vec4(shadowPos,1.0),bias,shadowMap0);
   float  shadow1 =shadowPoisson(frameUbo.lightViewProj[1] * vec4(shadowPos,1.0),bias,shadowMap1);
    shadow = blendCascade(shadow0, shadow1, frameUbo.cascadeSplit[0], blendWidth0, viewDepth);
 
    
    cascadeColor = vec3(1,0,0);
  } else if(viewDepth <= frameUbo.cascadeSplit[1]+blendWidth1 ) {
   float  shadow1 =shadowPoisson(frameUbo.lightViewProj[1] * vec4(shadowPos,1.0),bias,shadowMap1);
   float shadow2 =shadowPoisson(frameUbo.lightViewProj[2] * vec4(shadowPos,1.0),bias,shadowMap2);
    shadow = blendCascade(shadow1, shadow2, frameUbo.cascadeSplit[1], blendWidth1, viewDepth);   
    cascadeColor = vec3(0,1,0);
  } else if(viewDepth <= frameUbo.cascadeSplit[2]+blendWidth2) {
    float shadow2 =shadowPoisson(frameUbo.lightViewProj[2] * vec4(shadowPos,1.0),bias,shadowMap2);
    float  shadow3 =shadowPoisson(frameUbo.lightViewProj[3] * vec4(shadowPos,1.0),bias,shadowMap3);
    shadow = blendCascade(shadow2, shadow3, frameUbo.cascadeSplit[2], blendWidth2, viewDepth);
   
    cascadeColor = vec3(0,0,1);
  } else {
    shadow = shadowPoisson(frameUbo.lightViewProj[3] * vec4(shadowPos,1.0),bias,shadowMap3);
    cascadeColor = vec3(1,1,0);
  }
  CascadeResult result;
  result.shadow = shadow;
  result.cascadeColor = cascadeColor;
    return result;
}

vec3 mapToHeatColor(float value, float minVal, float maxVal, uint ramp) {
float t = clamp((value-minVal)/(maxVal-minVal),0.0, 1.0);
if(ramp == 0) return vec3(t);
if(ramp == 1) return mix(vec3(0.2,0.0,0.4), vec3(1.0,0.9,0.2), t);
return mix(vec3(0,0,1), vec3(1,0,0), t);
}

vec3 blingPhongShading(
  vec3 worldPos, 
vec3 normal, 
float shadow
) {
   vec3 L = normalize(frameUbo.lightPosition.xyz - worldPos);
   vec3 V = normalize(frameUbo.cameraPosition.xyz - worldPos);
   vec3 H = normalize(L + V);
    float diffuse = max(0.0, dot(normal,L));
  float specular = diffuse > 0.0 
    ? pow(max(0.0, dot(normal, H)), frameUbo.shininess) 
    : 0.0;

   vec3 lightColor = frameUbo.lightColor.xyz;
   vec3 ambientColor = (lightColor * (frameUbo.ambient) * frameUbo.Ka);
   vec3 diffuseColor = lightColor * diffuse;
   vec3 specularColor = lightColor * specular * frameUbo.Ks;   

   return (ambientColor + shadow * (diffuseColor + specularColor));
}

vec3 lambertDiffuse(vec3 albedo, vec3 N, vec3 L) {
float NdotL = max(0.0,dot(N, L));
return albedo / PI * NdotL;
}


float distributionGGX(vec3 N, vec3 H, float roughness) {
float a = roughness * roughness;
float a2 = a*a;
float NdotH = max(dot(N,H), 0.0);
float NdotH2 = NdotH * NdotH;
float denom = (NdotH2 * (a2 - 1.0) + 1.0);
denom = PI * denom * denom;

return a2/max(denom, 0.00001);
}

vec3 fresnelSchlick(vec3 Fo, float cosTheta) {
  return Fo + (1.0 -Fo) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float geometrySchlickGGX(float NdotV, float roughness) {
  float r = roughness + 1.0;
  float k = (r*r)/8.0;
  return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 L, vec3 V, float roughness) {
float ggx_V =geometrySchlickGGX(max(dot(N,V), 0.0), roughness);
float ggx_L =geometrySchlickGGX(max(dot(N,L), 0.0), roughness);

return ggx_L * ggx_V;
}

vec3 cookTorranceSpecular(vec3 N, vec3 V, vec3 L, vec3 H,
                           float roughness, vec3 F0) {
 float D = distributionGGX(N, H, roughness);
 vec3 F = fresnelSchlick(F0,max(dot(H,V),0.0));
 float G = geometrySmith(N,L,V,roughness);
 float NdotL = max(dot(N,L),0.0);
 float NdotV = max(dot(N,V),0.0);
 float denom = 4.0 * NdotL * NdotV + 0.00001;

 return (D*F*G)/ denom;
}

vec3 metallicDiffuse(vec3 F ,float metallic) {
  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
return kD * (1.0 - metallic);
}

vec3 sampleEnvironment(vec3 N, vec3 V, samplerCube envTexture) {
  vec3 R = reflect(-V,N);
 
 return texture(envTexture,R).rgb;
}

vec3 diffuseIBL(vec3 N, vec3 albedo, float metallic, samplerCube irradianceMap) {
    vec3 irradiance = texture(irradianceMap, N).rgb;

    vec3 kD = albedo * (1.0 - metallic);

    return kD * irradiance;
}

vec3 specularIBL(vec3 N, vec3 V, float roughness, vec3 F0,samplerCube prefilterEnv, sampler2D brdfLUTTex) {
vec3 R = reflect(-V, N);
float maxMip = 4.0; 
 vec3 prefilteredColor = textureLod(prefilterEnv, R, roughness * maxMip).rgb;

 float NdotV = max(dot(N, V), 0.0);
  vec2 envBRDF = texture(brdfLUT, vec2(NdotV, roughness)).rg;

 return prefilteredColor * (F0 * envBRDF.x + envBRDF.y); 

}


void main() {
  float depth   = texture(gDepth, fragUV).x;
  if(depth >= 1.0)
{
    vec3 color = texture(environment, fragUV).rgb;
    outColor = vec4(color,1.0);
    return;
}
  vec3 albedo = texture(gAlbedo, fragUV).rgb;
  vec3  worldPos = reconstructPosition(fragUV, depth, frameUbo.invViewProj);
  vec3 N = decodeNormal(texture(gNormal,fragUV).rg);
  vec3 material = texture(gMaterial, fragUV).rgb;
  CascadeResult cascadeResult =  getCascadeShadow(worldPos, N);

  float shadow= cascadeResult.shadow;
  vec3 cascadeColor = cascadeResult.cascadeColor;
float ao = material.r;

  vec3 finalColor; 
  // vec3 PLColor = texture(lightShading, fragUV).rgb;
  vec3 PLColor = vec3(0.0);

vec3 L =  normalize(frameUbo.lightPosition.xyz - worldPos);
vec3 V = normalize(frameUbo.cameraPosition.xyz - worldPos);
vec3 H = normalize(L + V);
vec3 directionalLighting;
float roughness = material.b;
float metallic = material.g;
vec3 dielectricF0 = vec3(0.04);
vec3 F0 = mix(dielectricF0, albedo, metallic);
float D = distributionGGX(N, H,roughness);
vec3 F = fresnelSchlick(F0, max(dot(N,V),0.0));
float G = geometrySmith(N, L, V,roughness);
// vec3 ambientFactor = vec3(1.0 - texture(ssaoTexture, fragUV).r);
vec3 ambientFactor = vec3(1.0);
vec3 ambient = diffuseIBL(N, albedo, metallic, irradianceTex) * ao * ambientFactor;
vec3 specularIBLColor = specularIBL(N, V, roughness, F0, prefilterMap, brdfLUT) * ao * ambientFactor;
switch(int(frameUbo.lightMode)) {
  case 0:
    directionalLighting = blingPhongShading(
   worldPos, 
  N, 
 shadow
);
    break;
  case 1:
    directionalLighting = lambertDiffuse(albedo, N,L);
    break;
  default:
   vec3 diffuse = metallicDiffuse(F, metallic) * lambertDiffuse(albedo,N,L);

    directionalLighting =(cookTorranceSpecular(N,L,V,H,material.b,F0) + diffuse);
    directionalLighting += ambient + specularIBLColor;
    break;
}
switch(int(frameUbo.debugMode)) {
  case 1:
    finalColor = albedo;
    break;
  case 2:
    finalColor = N;
    break;
  case 3:
    finalColor = vec3(worldPos.z);
    break;
  case 4:
    finalColor = vec3(worldPos * 0.05);
    break;
  case 5:
    finalColor = cascadeColor;
    break;
  case 6:
    finalColor = PLColor;
    break;
  case 7:
    finalColor = directionalLighting;
    break;
  case 8:
    finalColor = PLColor;
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
    finalColor = directionalLighting  + (PLColor * albedo) + texture(gEmissive, fragUV).rgb;
    break;
}


  outColor = vec4(
finalColor,
    1.0);

}

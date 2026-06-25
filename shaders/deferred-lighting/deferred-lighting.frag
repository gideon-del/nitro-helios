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
} frameUbo;


layout(set=1, binding=0) uniform sampler2D gAlbedo;
layout(set=1, binding=1) uniform sampler2D gNormal;
layout(set=1, binding=2) uniform sampler2D gMaterial;
layout(set=1, binding=3) uniform sampler2D gEmissive;
layout(set=1, binding=4) uniform sampler2D gDepth;
layout(set=1, binding=5) uniform sampler2D lightShading;

layout(set=2, binding=0) uniform sampler2DShadow shadowMap0;
layout(set=2, binding=1) uniform sampler2DShadow shadowMap1;
layout(set=2, binding=2) uniform sampler2DShadow shadowMap2;
layout(set=2, binding=3) uniform sampler2DShadow shadowMap3;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2  fragUV;
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
void main() {
  vec3 albedo = texture(gAlbedo, fragUV).rgb;
  float depth   = texture(gDepth, fragUV).x;
  if(depth >= 1.0)
{
   discard;
    return;
}
  vec3  worldPos = reconstructPosition(fragUV, depth, frameUbo.invViewProj);
  vec3 N = decodeNormal(texture(gNormal,fragUV).rg);
   vec3 L = normalize(frameUbo.lightPosition.xyz - worldPos);
   vec3 V = normalize(frameUbo.cameraPosition.xyz - worldPos);
   vec3 H = normalize(L + V);
    float diffuse = max(0.0, dot(N,L));
  float specular = diffuse > 0.0 
    ? pow(max(0.0, dot(N, H)), frameUbo.shininess) 
    : 0.0;

   float bias = max(
   frameUbo.shadowBias * (1.0 - dot(N, L)),
    0.0005
);

float normalBias =
    (1.0 - dot(N, L)) * frameUbo.shadowNormalBias;

vec3 shadowPos =
    worldPos + N * normalBias;

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

vec3 lightColor = frameUbo.lightColor.xyz;
   vec3 ambientColor = (lightColor * (frameUbo.ambient) * frameUbo.Ka);
   vec3 diffuseColor = lightColor * diffuse ;
   vec3 specularColor = lightColor * specular * frameUbo.Ks;
   vec3 finalColor; 
  vec3 PLColor = texture(lightShading, fragUV).rgb;



 vec3 directionalLighting = (ambientColor + shadow * (diffuseColor + specularColor));
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
    finalColor = PLColor*albedo;
    break;
  case 7:
    finalColor = directionalLighting * albedo;
    break;
  case 8:
    finalColor = PLColor;
    break;
  default:
    finalColor = (directionalLighting  + PLColor) * albedo;
    break;
}


  outColor = vec4(
finalColor,
    1.0);

}
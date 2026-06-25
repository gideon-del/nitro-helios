#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;




layout(set=1, binding=0) uniform sampler2DShadow shadowMap0;
layout(set=1, binding=1) uniform sampler2DShadow shadowMap1;
layout(set=1, binding=2) uniform sampler2DShadow shadowMap2;
layout(set=1, binding=3) uniform sampler2DShadow shadowMap3;

struct PointLight {
 vec4 position;
 vec4 color;
 float radius;
 float intensity;
};

layout(set=0, binding=2) uniform FrameUniformBuffer {
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 lightPos;
    vec4 lightColor;
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
    PointLight pointLights[1000]; 
   
} frameUbo;



float kernel_shadow(vec4 fragLightPos, float bias, sampler2DShadow shadowMap) {
   vec3 projCoords = fragLightPos.xyz / fragLightPos.w;
   projCoords.xy = projCoords.xy * 0.5 + 0.5;
   vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

   float shadow = 0.0;


   for(int y=-1; y <= 1; y++) {
     for(int x=-1; x <= 1; x++) {     
     vec2 offset = vec2(x, y) * texelSize;
      shadow += texture(shadowMap, vec3(projCoords.xy + offset, projCoords.z -bias));
   }
   }
   
   return shadow / 9.0;
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

void main() {

   vec3 N = normalize(fragNormal);
   vec3 L = normalize(frameUbo.lightPos.xyz - fragPos);
   vec3 V = normalize(frameUbo.cameraPos.xyz - fragPos);
   vec3 H = normalize(L + V);

   float diffuse = max(0.0, dot(N,L));
  float specular = diffuse > 0.0 
    ? pow(max(0.0, dot(N, H)), frameUbo.shininess) 
    : 0.0;

   vec3 lightColor = frameUbo.lightColor.xyz;

  float bias = max(
   frameUbo.shadowBias * (1.0 - dot(N, L)),
    0.0005
);

float normalBias =
    (1.0 - dot(N, L)) * frameUbo.shadowNormalBias;

vec3 shadowPos =
    fragPos + N * normalBias;

float shadow = 0.0;
vec4 viewPos =
    frameUbo.view * vec4(fragPos, 1.0);

float viewDepth = -viewPos.z;
vec3 cascadeColor;
float blendWidth0 = (frameUbo.cascadeSplit[1] - frameUbo.cascadeSplit[0] ) * 0.15;
float blendWidth1 = (frameUbo.cascadeSplit[2] - frameUbo.cascadeSplit[1] ) * 0.15;
float blendWidth2 = (frameUbo.cascadeSplit[3] - frameUbo.cascadeSplit[2] ) * 0.15;
float blend =0.0;
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

   vec3 ambientColor = (lightColor * (frameUbo.ambient) * frameUbo.Ka);
   vec3 diffuseColor = lightColor * diffuse ;
   vec3 specularColor = lightColor * specular * frameUbo.Ks;
   vec3 color = vec3(0.4,0.5,0.9);
   vec3 finalColor; 


  vec3 PLColor = vec3(0.0);
for(int i =0; i < 1000; i++) {
   vec3 PL = frameUbo.pointLights[i].position.xyz - fragPos;
  float dist = length(PL);

  if(dist <= frameUbo.pointLights[i].radius ) {
    PL = normalize(PL);
 float attenuation = pow(max(0.0, 1.0 - pow(dist/frameUbo.pointLights[i].radius, 4)), 2)
   / (dist * dist);
 
    float diffuse = max(0.0, dot(N, PL));
    PLColor += color * frameUbo.pointLights[i].color.xyz * diffuse * attenuation * frameUbo.pointLights[i].intensity;
  }
}
vec3 directionalLighting = (ambientColor + shadow * (diffuseColor + specularColor));
switch(int(frameUbo.debugMode)) {
  case 1:
    finalColor = color;
    break;
  case 2:
    finalColor = N;
    break;
  case 3:
    finalColor = vec3(fragPos.z);
    break;
  case 4:
    finalColor = vec3(fragPos * 0.05);
    break;
  case 5:
    finalColor = cascadeColor;
    break;
  case 6:
    finalColor = PLColor;
    break;
  case 7:
    finalColor = directionalLighting * color;
    break;
  default:
    finalColor = (directionalLighting * color) + PLColor;
    break;
}


    outColor = vec4( finalColor,1.0);
  //  outColor =  vec4(frameUbo.pointLight.radius / 20.0);
}
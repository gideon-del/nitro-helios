#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragLightPos;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;




layout(set=0, binding=0) uniform sampler2DShadow shadowMap;

layout(set=0, binding=2) uniform FrameUniformBuffer {
    mat4 view;
    mat4 proj;

    vec4 cameraPos;
    
    vec4 lightPos;
    vec4 lightColor;

    mat4 lightViewProj;

     float ambient;
    float Ka;
    float Kd;
    float Ks;
    float shininess;
} frameUbo;

layout(set=0, binding=5) uniform CameraView {
 vec4 position;
}camera;

float kernel_shadow(vec4 fragLightPos, float bias) {
   vec3 projCoords = fragLightPos.xyz / fragLightPos.w;
   projCoords.xy = projCoords.xy * 0.5 + 0.5;
   vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

   float shadow = 0.0;


   for(int y=-1; y <= 1; y++) {
     for(int x=-1; x <= 1; x++) {     
     vec2 offset = vec2(x, y) * texelSize;
      shadow += texture(shadowMap, vec3(projCoords.xy + offset, projCoords.z - bias));
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

float shadowPoisson(vec4 fragLightPos, float bias) {
     vec3 projCoords = fragLightPos.xyz / fragLightPos.w;
   projCoords.xy = projCoords.xy * 0.5 + 0.5;
   vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

   float shadow = 0.0;
   float radius = 2.0;
   for(int i =0; i < 16; i++) {
     shadow += texture(shadowMap,vec3(projCoords.xy + poissonDisk[i] * texelSize *radius, projCoords.z - bias));
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
    0.005 * (1.0 - dot(N, L)),
    0.0005
);

//  vec3 projCoords = fragLightPos.xyz / fragLightPos.w;
//    projCoords.xy = projCoords.xy * 0.5 + 0.5;
//   float closestDepth = texture(shadowMap,projCoords.xy).r;
//   float currentDepth = projCoords.z;

  float shadow = shadowPoisson(fragLightPos,bias);
  // float shadow = shadowPoisson(fragLightPos,bias);

// float shadow = (currentDepth -bias) > closestDepth ? 0.0 : 1.0;
   vec3 ambientColor = (lightColor * (frameUbo.ambient) * frameUbo.Ka);
   vec3 diffuseColor = lightColor * diffuse ;
   vec3 specularColor = lightColor * specular * frameUbo.Ks;
   vec3 color = vec3(0.4,0.5,0.9);
   vec3 finalColor = (ambientColor + shadow * (diffuseColor + specularColor)) * color; 

    outColor = vec4(finalColor,1.0);
}
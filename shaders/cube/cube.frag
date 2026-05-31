#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragLightPos;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;




layout(set=0, binding=0) uniform sampler2D shadowMap;

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

  

    vec3 projCoords = fragLightPos.xyz / fragLightPos.w;
    // projCoords= projCoords * 0.5 + 0.5;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).x;
    float currentDepth = projCoords.z;

//   float bias = max(
//     0.005 * (1.0 - dot(N, L)),
//     0.0005
// );
  float bias =0.0005;


  float shadow = (currentDepth - bias) > closestDepth ? 0.0 : 1.0;


 vec3 ambientColor = (lightColor * (frameUbo.ambient) * frameUbo.Ka);
   vec3 diffuseColor = lightColor * diffuse ;
   vec3 specularColor = lightColor * specular * frameUbo.Ks;
  vec3 color = vec3(0.4,0.5,0.9);

    vec3 finalColor = (ambientColor + shadow * (diffuseColor + specularColor)) * color; 

  float diff = abs(currentDepth - closestDepth) * 50.0;
    outColor = vec4(finalColor,1.0);
}
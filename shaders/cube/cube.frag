#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragLightPos;

layout(location = 0) out vec4 outColor;




layout(set=0, binding=0) uniform sampler2D shadowMap;
void main() {

    vec3 projCoords = fragLightPos.xyz / fragLightPos.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
  float bias = 0.005;
   float shadow =
    currentDepth-bias > closestDepth
    ? 0.0
    : 1.0;

    vec3 color = vec3(fragUV,0.0) * shadow;
    outColor = vec4(color,1.0);
}
#version 450
layout(location =0) out vec4 outColor;


layout(set=0,binding=3) uniform sampler2D gDepth;
layout(set=0,binding=4) uniform sampler2D gNormal;
layout(push_constant) uniform PushConstant {
    mat4 model;
    vec4 lightPosition;
    vec4 lightColor;
    vec2 screenSize;
    float radius;
    float intensity;
} pc;

layout(set=0, binding=2) uniform CameraVP {
   mat4 view;
   mat4 proj;
   mat4 invViewProj;
} camera;

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

void main() {
    vec2 uv = gl_FragCoord.xy / pc.screenSize;
    float depth = texture(gDepth, uv).r;
    vec3 normal = decodeNormal(texture(gNormal,uv).xy);
    vec3 worldPos = reconstructPosition(uv, depth, camera.invViewProj);
     vec3 PL = pc.lightPosition.xyz - worldPos;
  float dist = length(PL);
PL = normalize(PL);
    float attenuation = pow(max(0.0, 1.0 - pow(dist/pc.radius, 4)), 2)
   / (dist * dist);
   float diffuse = max(0.0, dot(normal,PL));

   vec3 finalColor = pc.lightColor.xyz * diffuse * attenuation * pc.intensity;
   outColor = vec4(finalColor,1.0);
}
#version 450


#define DEBUG_NONE     0
#define DEBUG_GGX      1
#define DEBUG_FRESNEL  2
#define DEBUG_GEOMETRY 3
#define DEBUG_AMBIENT  4
#define DEBUG_DIRECT   5


layout(set = 0, binding = 0) uniform LightingFrame {
    mat4 invViewProj;
    vec4 cameraPosition;
    vec4 lightDirection;  
    vec4 lightColor;     
    vec4 ambient;         
    vec4 skyColor;
    uint debugMode;
} frame;


layout(set=0, binding=1) uniform sampler2D gAlbedo;
layout(set=0, binding=2) uniform sampler2D gNormal;
layout(set=0, binding=3) uniform sampler2D gMaterial;
layout(set=0, binding=4) uniform sampler2D gEmissive;
layout(set=0, binding=5) uniform sampler2D gDepth;

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


vec3 mapToHeatColor(float value, float minVal, float maxVal, uint ramp) {
float t = clamp((value-minVal)/(maxVal-minVal),0.0, 1.0);
if(ramp == 0) return vec3(t);
if(ramp == 1) return mix(vec3(0.2,0.0,0.4), vec3(1.0,0.9,0.2), t);
return mix(vec3(0,0,1), vec3(1,0,0), t);
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



void main() {
ivec2 p = ivec2(gl_FragCoord.xy);
float depth = texelFetch(gDepth, p, 0).r;
if (depth >= 1.0) { outColor = vec4(frame.skyColor.rgb, 1); return; }

vec3 albedo = texelFetch(gAlbedo, p, 0).rgb;
vec3 N = decodeNormal(texelFetch(gNormal, p,0).rg);

vec3 m = texelFetch(gMaterial, p, 0).rgb;

float ao = m.r, metallic = m.g, roughness = m.b;

vec3 worldPos = reconstructPosition(fragUV, depth, frame.invViewProj);
vec3 V = normalize(frame.cameraPosition.xyz - worldPos);
vec3 L = normalize(-frame.lightDirection.xyz);
vec3 H = normalize(L + V);

float NdotL = max(dot(N,L),0.0), NdotV = max(dot(N,V), 0.0);


vec3 F0 = mix(vec3(0.04), albedo, metallic);
float D = distributionGGX(N, H, roughness);
float G = geometrySmith(N, L, V, roughness);
vec3 F = fresnelSchlick(F0, max(dot(H, V), 0.0));


vec3 specular = ( D * F * G) / max(4 * NdotL * NdotV, 1e-4);


vec3 kD = (1.0 - F) * (1.0 - metallic);
vec3 radiance = frame.lightColor.rgb * frame.lightColor.a;


vec3 direct = (kD * albedo / PI + specular) * radiance * NdotL;


vec3 ambient = frame.ambient.rgb * frame.ambient.a * albedo * (1.0 - metallic) * ao;

vec3 color = direct + ambient + texelFetch(gEmissive, p, 0).rgb;


vec3 result;
switch (frame.debugMode)
{
    case DEBUG_GGX:      result = mapToHeatColor(D, 0.0, 1.0, 2); break;
    case DEBUG_FRESNEL:  result = F; break;
    case DEBUG_GEOMETRY: result = vec3(G); break;
    case DEBUG_AMBIENT:  result = ambient; break;
    case DEBUG_DIRECT:   result = direct; break;
    default:             result = color; break;
}
outColor = vec4(result, 1.0);



}

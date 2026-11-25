#version 450


#define DEBUG_NONE     0
#define DEBUG_GGX      1
#define DEBUG_FRESNEL  2
#define DEBUG_GEOMETRY 3
#define DEBUG_AMBIENT  4
#define DEBUG_DIRECT   5
#define DEBUG_DiffuseIBL   6
#define DEBUG_SpecularIBL   7
#define DEBUG_CASCADE 8



layout(set = 0, binding = 0) uniform LightingFrame {
    mat4 invViewProj;
    vec4 cameraPosition;
    vec4 lightDirection;  
    vec4 lightColor;     
    vec4 ambient;         
    vec4 skyColor;
    uint debugMode;
    uint prefilterMipCount;
    float envIntensity;
    bool hasEnvironment;
} frame;


layout(set=0, binding=1) uniform sampler2D gAlbedo;
layout(set=0, binding=2) uniform sampler2D gNormal;
layout(set=0, binding=3) uniform sampler2D gMaterial;
layout(set=0, binding=4) uniform sampler2D gEmissive;
layout(set=0, binding=5) uniform sampler2D gDepth;

layout(set = 0, binding = 6) uniform samplerCube environment;
layout(set = 0, binding = 7) uniform samplerCube irradiance;
layout(set = 0, binding = 8) uniform samplerCube prefiltered;
layout(set = 0, binding = 9) uniform sampler2D   brdfLut;

layout(set = 0, binding = 10) uniform ShadowFrame {
    mat4  cascadeViewProj[4];
    mat4  view;            
    vec4  splits;         
    vec4  params; 
    vec4 texelWorld;         
} shadow;


layout(set=0, binding=11) uniform sampler2DShadow shadowMap0;
layout(set=0, binding=12) uniform sampler2DShadow shadowMap1;
layout(set=0, binding=13) uniform sampler2DShadow shadowMap2;
layout(set=0, binding=14) uniform sampler2DShadow shadowMap3;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2  fragUV;

const float PI = 3.14159265358979323846;
const float TWO_PI = 6.28318530717958647692;
const float HALF_PI = 1.57079632679489661923;
const vec3 kCascadeTint[4] = vec3[](vec3(1,0.2,0.2), vec3(0.2,1,0.2), vec3(0.2,0.4,1), vec3(1,1,0.2));
vec3 reconstructPosition(vec2 uv, float depth, mat4 invViewProj) {
 vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
 vec4 worldPos = invViewProj * clipPos;
 return worldPos.xyz /worldPos.w;
}


float sampleCascade(int c, vec3 uvz) {
  switch(c){
    case 0: return  texture(shadowMap0, uvz);
    case 1: return  texture(shadowMap1, uvz);
    case 2: return  texture(shadowMap2, uvz);
   default: return  texture(shadowMap3, uvz);
  }
}

int selectCascade(vec3 worldPos) {
    float viewDepth = -(shadow.view * vec4(worldPos, 1.0)).z;
    for (int i = 0; i < 4; ++i)
        if (viewDepth < shadow.splits[i]) return i;
    return 3;
}

float computeShadow(vec3 worldPos, vec3 N, vec3 L){

   float viewDepth = -(shadow.view * vec4(worldPos, 1.0)).z;

   int c = 3;

   for(int i =0; i < 4; ++i) {
     if(viewDepth < shadow.splits[i]){
      c =i; 
      break;
     }
   }
   

   float NdotL = clamp(dot(N,L), 0.0, 1.0);
    vec3 p = worldPos + N * shadow.params.x * shadow.texelWorld[c] * (1.0 - NdotL);

    vec4 clip = shadow.cascadeViewProj[c] * vec4(p, 1.0);
    vec3 ndc  = clip.xyz / clip.w;
    vec2 uv   = ndc.xy * 0.5 + 0.5;
    if (any(lessThan(uv, vec2(0))) || any(greaterThan(uv, vec2(1))) || ndc.z > 1.0) return 1.0;
    

    float texel = shadow.params.y, sum = 0.0;
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            sum += sampleCascade(c, vec3(uv + vec2(x, y) * texel, ndc.z));
    return sum / 9.0;
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
if (depth >= 1.0) { 

    vec4 farP = frame.invViewProj * vec4(fragUV * 2.0 -1.0,1.0,1.0);
    vec3 dir = normalize(farP.xyz / farP.w - frame.cameraPosition.xyz);
    vec3 sky = textureLod(environment, dir, 0.0).rgb * frame.envIntensity;
  outColor = vec4(sky, 1.0);
    return;
 }

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


vec3 R = reflect(-V, N);
vec3 Fr = F0 + (max(vec3(1.0-roughness), F0) - F0) * pow(1.0 - NdotV, 5.0);
vec3 kD   = (1.0 - Fr) * (1.0 - metallic);

vec3 diffuseIBL  = kD * albedo * texture(irradiance, N).rgb;
vec3 prefilt     = textureLod(prefiltered, R, roughness * float(frame.prefilterMipCount - 1)).rgb;
vec2 brdf        = texture(brdfLut, vec2(NdotV, roughness)).rg;
vec3 specularIBL = prefilt * (F0 * brdf.x + brdf.y);

vec3 radiance = frame.lightColor.rgb * frame.lightColor.a;


vec3 kDdirect = (1.0 - F) * (1.0 - metallic);

vec3 direct = (kDdirect * albedo / PI + specular) * radiance * NdotL;
vec3 geoN = normalize(cross(dFdx(worldPos), dFdy(worldPos)));

if(dot(geoN, V)  <0.0) geoN = -geoN;
direct *= computeShadow(worldPos, geoN, L);
vec3 ambient  = (diffuseIBL + specularIBL) * ao * frame.envIntensity;

vec3 color = direct + ambient + texelFetch(gEmissive, p, 0).rgb;





vec3 result;
switch (frame.debugMode)
{
    case DEBUG_GGX:      result = mapToHeatColor(D, 0.0, 1.0, 2); break;
    case DEBUG_FRESNEL:  result = F; break;
    case DEBUG_GEOMETRY: result = vec3(G); break;
    case DEBUG_AMBIENT:  result = ambient; break;
    case DEBUG_DIRECT:   result = direct; break;
    case DEBUG_DiffuseIBL:   result = diffuseIBL; break;
    case DEBUG_SpecularIBL:   result = specularIBL; break;
    case DEBUG_CASCADE:   result = kCascadeTint[selectCascade(worldPos)] * (0.25 + 0.75 * NdotL); break;; break;
    default:             result = color; break;
}

outColor = vec4(result, 1.0);



}

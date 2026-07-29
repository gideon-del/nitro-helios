#version 450


layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragTangent;


layout(set=1, binding=0) uniform sampler2D baseColorTexture;
layout(set=1, binding=1) uniform sampler2D normalTexture;
layout(set=1, binding=2) uniform sampler2D metallicRoughnessTexture;
layout(set=1, binding=3) uniform sampler2D aoTexture;
layout(set=1, binding=4) uniform sampler2D emissiveTexture;


layout(location =0) out vec4 gAlbedo;
layout(location =1) out vec4 gNormal;
layout(location = 2) out vec4 gMaterial;  
layout(location = 3) out vec4 gEmissive;

layout(push_constant)uniform PushConstant {
    mat4 model;
    mat4 normal;
    vec4 baseColor;
    float metallic;
    float roughness;
   uint useTextures;
} pc;

vec2 encodeNormal(vec3 n) {
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    if (n.z < 0.0)
        return (1.0 - abs(n.yx)) * sign(n.xy) * 0.5 + 0.5;
    return n.xy * 0.5 + 0.5;
}

void main() {
 vec3 N = normalize(fragNormal);
    vec3 T = normalize(fragTangent.xyz);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N,T) * fragTangent.w;
    mat3 TBN = mat3(T,B,N);
    if(pc.useTextures == 1) {
    vec3 metallicRoughness = texture(metallicRoughnessTexture, fragUV).rgb;
    vec4 color = texture(baseColorTexture, fragUV);
    vec3 tangentNormal = texture(normalTexture, fragUV).rgb * 2.0 - 1.0;
    tangentNormal = normalize(tangentNormal);
    vec3 worldNormal = normalize(TBN * tangentNormal);
    gAlbedo =color;
    gNormal = vec4(encodeNormal(worldNormal), 0.0,1.0);
    float ao = texture(aoTexture, fragUV).r;
    gMaterial = vec4(ao,metallicRoughness.b,metallicRoughness.g,1.0);
    gEmissive = vec4(texture(emissiveTexture, fragUV).rgb,1.0);
    }else {
    gAlbedo = pc.baseColor;
    gNormal = vec4(encodeNormal(fragNormal), 0.0,1.0);
    gMaterial = vec4(1.0,pc.metallic,pc.roughness,1.0);
    gEmissive = vec4(vec3(0.0),1.0);
    }
  
}
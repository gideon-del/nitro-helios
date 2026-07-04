#version 450


layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;


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

    if(pc.useTextures == 1) {
    vec3 metallicRoughness = texture(metallicRoughnessTexture, fragUV).rgb;
    gAlbedo = texture(baseColorTexture, fragUV);
    gNormal = vec4(encodeNormal(fragNormal), 0.0,1.0);
    gMaterial = vec4(0.0,metallicRoughness.b,metallicRoughness.g,1.0);
    gEmissive = vec4(1.0,1.0,1.0,1.0);
    }else {
    gAlbedo = pc.baseColor;
    gNormal = vec4(encodeNormal(fragNormal), 0.0,1.0);
    gMaterial = vec4(0.0,pc.metallic,pc.roughness,1.0);
    gEmissive = vec4(1.0,1.0,1.0,1.0);
    }
  
}
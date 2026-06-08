#version 450


layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;


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
} frameUbo;
layout(push_constant)uniform PushConstant {
    mat4 model;
    mat4 normal;
} pc;
void main() {
    vec4 worldPos = pc.model * vec4(aPos, 1.0);
    gl_Position = frameUbo.proj * frameUbo.view * worldPos;

    
    fragUV = aUV;
    
    mat3 normalMatrix = {
        pc.normal[0].xyz,
        pc.normal[1].xyz,
        pc.normal[2].xyz
    };

    fragNormal = normalMatrix * aNormal;
    fragPos = worldPos.xyz;
}
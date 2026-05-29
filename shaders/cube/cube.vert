#version 450


layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragLightPos;



layout(set=0, binding=2) uniform UniformBuffer {
    mat4 view;
    mat4 proj;
} ubo;
layout(set=0, binding=3) uniform LightUBO {
    mat4 lightViewProj;
} lt;
layout(push_constant)uniform PushConstant {
    mat4 model;
    mat4 normal;
} pc;
void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(aPos, 1.0);

    
    fragUV = aUV;
    fragLightPos = lt.lightViewProj * pc.model *vec4(aPos, 1.0);
}
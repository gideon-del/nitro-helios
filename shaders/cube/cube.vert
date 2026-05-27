#version 450



layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;

layout(set=0, binding=2) uniform UniformBuffer {
    mat4 view;
    mat4 proj;
} ubo;
layout(push_constant)uniform PushConstant {
    mat4 model;
    mat4 normal;
} pc;
void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(aPos, 1.0);
    mat3 normalMatrix = {
        pc.normal[0].xyz,
        pc.normal[1].xyz,
        pc.normal[2].xyz,
    };
    fragColor = aColor;
    fragNormal = normalMatrix * aNormal;
}
#version 450


layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragNormal;



layout(set=0, binding=2) uniform GeometryUBO {
    mat4 view;
    mat4 proj;
} gUbo;

layout(push_constant)uniform PushConstant {
    mat4 model;
    mat4 normal;
} pc;


void main() {
    gl_Position = gUbo.proj * gUbo.view * pc.model * vec4(aPos, 1.0);

    fragUV = aUV;
     mat3 normalMatrix = {
        pc.normal[0].xyz,
        pc.normal[1].xyz,
        pc.normal[2].xyz
    };

    fragNormal = normalMatrix * aNormal;
}
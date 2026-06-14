#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aUV;


layout(set =0, binding=2) uniform Camera {
 mat4 view;
 mat4 proj;
} camera;

layout(push_constant) uniform PushConstant {
    mat4 model;
    mat4 normal;
} pc;


void main() {   
    gl_Position = camera.proj * camera.view * pc.model * vec4(aPos, 1.0);
}
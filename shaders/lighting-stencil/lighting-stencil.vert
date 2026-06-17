#version 450

layout(location=0) in vec3 aPos;
layout(location=0) out vec3 fragPos;
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

void main() {
    
    gl_Position = camera.proj * camera.view * pc.model * vec4(aPos,1.0);
    
    fragPos =  (pc.model * vec4(aPos,1.0)).xyz;
}


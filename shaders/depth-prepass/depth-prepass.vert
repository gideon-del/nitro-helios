#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aUV;

  struct MeshInstance
    {
        uint meshId;
        uint materialId;
        mat4 modelTransform;
        mat4 normalTransform;           
    };

layout(set =0, binding=2) uniform Camera {
 mat4 view;
 mat4 proj;
} camera;

layout(std430, set=0, binding=3) readonly buffer MeshInstanceBuffer {
 MeshInstance meshInstances[];
}; 

invariant gl_Position;

void main() {   
     MeshInstance instance = meshInstances[gl_InstanceIndex];
    gl_Position = camera.proj * camera.view * instance.modelTransform * vec4(aPos, 1.0);
}
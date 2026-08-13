#version 450

layout(location =0) in vec3 aPos;
layout(location =1) in vec3 aColor;
layout(location =2) in vec3 aNormal;
layout(location =3) in vec2 aUV;

   struct MeshInstance
    {
        uint meshId;
        uint materialId;
        mat4 modelTransform;
        mat4 normalTransform;           
    };

layout(set =0, binding=2) uniform LightTransform {
    mat4 lightSpaceView[4];
} lt;
layout(std430, set=0, binding=3) readonly buffer MeshInstanceBuffer {
 MeshInstance meshInstances[];
}; 

layout(push_constant) uniform PushConstant {
    int cascadeIndex;
} pc;

void main() {
    MeshInstance instance = meshInstances[gl_InstanceIndex];
 gl_Position = lt.lightSpaceView[pc.cascadeIndex] * instance.modelTransform * vec4(aPos,1.0);   
}


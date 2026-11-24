#version 450

layout(location =0) in vec3 aPos;

   struct MeshInstance
    {
        uint meshId;
        uint materialId;
         float pads[2];
        mat4 modelTransform;
        mat4 normalTransform;           
    };


layout(std430, set=0, binding=0) readonly buffer MeshInstanceBuffer {
 MeshInstance meshInstances[];
}; 

layout(push_constant) uniform PushConstant {
    mat4 viewProj;
} pc;

void main() {
 gl_Position = pc.viewProj * meshInstances[gl_InstanceIndex].modelTransform * vec4(aPos,1.0);   
}


#version 450


layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aUV;
layout(location = 4) in vec4 aTangent;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragTangent;

   struct MeshInstance
    {
        uint meshId;
        uint materialId;
        mat4 modelTransform;
        mat4 normalTransform;           
    };


layout(set=0, binding=2) uniform GeometryUBO {
    mat4 view;
    mat4 proj;
} gUbo;

layout(std430, set=0, binding=3) readonly buffer MeshInstanceBuffer {
 MeshInstance meshInstances[];
}; 



void main() {

    MeshInstance instance = meshInstances[gl_InstanceIndex];
    gl_Position = gUbo.proj * gUbo.view * instance.modelTransform * vec4(aPos, 1.0);

    fragUV = aUV;
     mat3 normalMatrix = {
        instance.normalTransform[0].xyz,
        instance.normalTransform[1].xyz,
        instance.normalTransform[2].xyz
    };

    fragNormal = normalMatrix * aNormal;
    fragTangent = vec4(normalMatrix * aTangent.xyz, aTangent.w);
}
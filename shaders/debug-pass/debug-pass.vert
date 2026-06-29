#version 450
struct DebugVertex {
 vec4 position;
 vec4 color;
};


layout(std430, set=0, binding=2) readonly buffer DebugVertices {
 DebugVertex debugVertices[];
};

layout(push_constant) uniform PushConstant {
    mat4 viewProj;
} pc;

layout(location =0) out vec3 fragColor;

void main() {
   gl_Position = pc.viewProj * debugVertices[gl_VertexIndex].position;

   fragColor= debugVertices[gl_VertexIndex].color.xyz;
}
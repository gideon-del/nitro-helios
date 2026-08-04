#version 450

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
struct Particle {
       vec4 position;
       vec4 velocity;
        vec4 color;
        float lifetime;
        float age;
        float size;
};

layout(std430, set=0, binding = 2) readonly buffer ParticleBuffer {
    Particle particles[];
};



layout(set=0, binding=3) uniform Camera {
 mat4 view;
 mat4 proj;
 vec4 up;
 vec4 right;
} cam;

layout(std430,set=0, binding=4)readonly  buffer AliveIndices {
    uint indices[];
} aliveList;

void main() {
  vec2 offsets[6] = vec2[](
    vec2(-0.5,-0.5), vec2(0.5,-0.5), vec2(-0.5,0.5),
    vec2(0.5,-0.5),  vec2(0.5,0.5),  vec2(-0.5,0.5)
);
uint id = gl_InstanceIndex;
   uint particleIdx = aliveList.indices[id];
   uint cornerId = gl_VertexIndex;

   Particle particle = particles[particleIdx];
   vec2 corner = offsets[cornerId];

   vec3 worldPos = particle.position.xyz + (cam.up.xyz * corner.y * particle.size) 
   + (cam.right.xyz * corner.x * particle.size); 

   gl_Position = cam.proj * cam.view * vec4(worldPos, 1.0);

   fragColor = particle.color;
   fragUV = corner + 0.5;
}
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 0) out vec4 outColor;

void main() {

     vec3 N = normalize(fragNormal);
    vec3 L = normalize(vec3(1.0,1.0,0.0));
    
    float intensity = max(0.0, dot(N,L));
    vec3 color = fragColor * intensity;
    outColor = vec4(color, 1.0);
}
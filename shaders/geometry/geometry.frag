#version 450


layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;



layout(location =0) out vec4 gAlbedo;
layout(location =1) out vec4 gNormal;
layout(location = 2) out vec4 gMaterial;  
layout(location = 3) out vec4 gEmissive;


vec2 encodeNormal(vec3 n) {
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    if (n.z < 0.0)
        return (1.0 - abs(n.yx)) * sign(n.xy) * 0.5 + 0.5;
    return n.xy * 0.5 + 0.5;
}

void main() {
    gAlbedo = vec4(0.4,0.5,0.9,1.0);
    gNormal = vec4(encodeNormal(fragNormal), 0.0,1.0);
    gMaterial = vec4(1.0,1.0,1.0,1.0);
    gEmissive = vec4(1.0,1.0,1.0,1.0);
}
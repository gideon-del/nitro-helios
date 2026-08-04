#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location =0) out vec4 outColor;

void main() {
    vec2 center = fragUV - 0.5;
    float dist = length(center) * 2.0;
    float alpha = 1.0 - smoothstep(0.0, 1.0, dist);

    if(alpha <= 0.0){
        discard;
    }
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
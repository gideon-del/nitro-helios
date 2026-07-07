#version 450
layout(location=0) in vec2 fragUV;
layout(location=0) out vec4 outColor;


layout(set=0, binding=2) uniform sampler2D finalTexture;

void main() {
    vec3 finalColor = texture(finalTexture, fragUV).rgb;
    outColor = vec4(finalColor, 1.0);
}
#version 450
layout(location=0) out vec4 outColor;

layout(set=0, binding=3) uniform samplerCube skyboxTexture;

layout(set=0, binding=2) uniform Camera {
    mat4 invViewProj;
    vec2 screenSize;
} camera;

void main() {
    vec2 uv = gl_FragCoord.xy / camera.screenSize;
    vec2 ndc = uv * 2.0 - 1.0;
ndc.y = -ndc.y;
    vec4 fragDir = camera.invViewProj * vec4(ndc, 1.0,1.0);
    vec3 finalDir = fragDir.xyz / fragDir.w;
    vec3 finalColor = texture(skyboxTexture, normalize(finalDir)).rgb;
    outColor = vec4(finalColor, 1.0);
}
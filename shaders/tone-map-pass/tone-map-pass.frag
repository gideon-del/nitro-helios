#version 450
layout(location=0) in vec2 fragUV;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstant {
 float exposure;
 uint mode;
} pc;

layout(set=0,binding=2) uniform sampler2D hdrTexture;


vec3 reinhard(vec3 color) {
    return (color) / (color + vec3(1.0));
}

vec3 acesApprox(vec3 color) {
    float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((color*(a*color+b))/(color*(c*color+d)+e), 0.0, 1.0);
}

vec3 gammaCorrect(vec3 color) { 
    return pow(color, vec3(1.0/2.2));
 }

 void main() {
    vec3 hdrColor = texture(hdrTexture, fragUV).rgb;
    hdrColor *= pc.exposure;
    
    vec3 mapped;
    switch(pc.mode) {
        case 1:
          mapped = reinhard(hdrColor);
          break;
        case 2:
          mapped = acesApprox(hdrColor);
          break;
        default:
          mapped = clamp(hdrColor, 0.0, 1.0);  
        break;
    }
    outColor = vec4(gammaCorrect(mapped),1.0);
 }
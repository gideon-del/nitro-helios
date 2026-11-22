#version 450
#define MODE_DEPTH  0
#define MODE_ALBEDO  1
#define MODE_NORMAL  2
#define MODE_MATERIAL  3
#define MODE_EMISSIVE  4
#define MODE_WORLDPOS  5

layout(location=0) in vec2 fragUV;
layout(location=0) out vec4 outColor;

layout(binding = 0) uniform sampler2D source;
layout(push_constant) uniform PC {
 mat4  invViewProj;   
    uint  mode;
    float near;
    float far;
};

vec3 octDecode(vec2 n) { 
   
 vec2 f = n * 2.0 - 1.0;
vec3 v = vec3(
    f.x,
    f.y,
    1.0 - abs(f.x) - abs(f.y));

if (v.z < 0.0)
{
    v.xy =
        (1.0 - abs(v.yx))
        * sign(v.xy);
}       

return normalize(v);
}


vec3 linearizeDepth(float d){
    float linear = near * far / (far - d * (far - near));  
    float v = (linear - near) / (far - near);

    return vec3(v);
}


vec3 reconstruct(float depth,vec2 uv) {
 vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
 vec4 worldPos = invViewProj * clipPos;
 return worldPos.xyz /worldPos.w;
}

void main() {
   vec4 s = texelFetch(source, ivec2(gl_FragCoord.xy), 0);
    vec3 c = s.rgb;

    switch(mode) {
        case MODE_DEPTH: c = linearizeDepth(s.r); break;
        case MODE_ALBEDO: c = s.rgb; break;
        case MODE_NORMAL:   c = octDecode(s.rg) * 0.5 + 0.5; break;
        case MODE_MATERIAL: c = vec3(s.g, s.b, 0.0); break;
        case MODE_EMISSIVE: c = s.rgb / (1.0 + s.rgb); break;
        case MODE_WORLDPOS: c = fract(reconstruct(s.r, fragUV)); break;
    }
    
    outColor = vec4(vec3(c), 1.0);
}
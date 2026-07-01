#version 450
layout(location =0) out vec4 outColor;

struct PointLight {
 vec4 position;
 vec4 color;
 float radius;
 float intensity;
};

struct TileDebug
{
    uint lightCount;
    float minDepth;
    float maxDepth;
    float tileNear;
    float tileFar;
    uint overflow;
};
layout(set=0, binding=2) uniform FrameUniformBuffer {
   mat4 invViewProj;
   mat4 view;
   vec2 screenSize;
   uint numTilesX;
   uint maxLightPerTile;
   uint showHeatMap;
} frameUBO;

layout(set=0,binding=3) uniform sampler2D gDepth;
layout(set=0,binding=4) uniform sampler2D gNormal;
layout(std430,set=0, binding=5) readonly buffer LightBuffer {
 PointLight pointLights[];
};
layout(std430,set=0, binding=6) readonly buffer TileLightCounts {
    uint tileLightCount[];
};
layout(std430,set=0, binding=7) readonly buffer TileLightIndices {
    uint tileLightIndices[];
};
layout(std430,set=0, binding=8) buffer TileDebugBuffer {
    TileDebug debugTiles[];
};

vec3 reconstructPosition(vec2 uv, float depth, mat4 invViewProj) {
 vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
 vec4 worldPos = invViewProj * clipPos;
 return worldPos.xyz / worldPos.w;
}


vec3 decodeNormal(vec2 n) {
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

void main() {
    vec2 uv = gl_FragCoord.xy / frameUBO.screenSize;
     float depth = texture(gDepth, uv).r;
        vec3 normal = decodeNormal(texture(gNormal,uv).xy);
    vec3 worldPos = reconstructPosition(uv, depth, frameUBO.invViewProj);
ivec2 tileId = ivec2(gl_FragCoord.xy) / 16;
    uint tile = tileId.y * frameUBO.numTilesX + tileId.x;
    uint lightCount = tileLightCount[tile];
     TileDebug tileInfo = debugTiles[tile];
  
     uint baseIdx = tile * frameUBO.maxLightPerTile;

     vec3 PLColor = vec3(0.0);
     if(frameUBO.showHeatMap == 1) {
        float t = clamp(lightCount / 32.0, 0.0, 1.0);
    vec3 color =
    mix(vec3(0,0,1),      
    vec3(0,1,0), t);
         outColor = vec4(color, 1.0);
            return;
     }
    for(uint i=0; i < lightCount; i++) {
        
       uint lightIdx =tileLightIndices[baseIdx+i];
       PointLight light = pointLights[lightIdx];
       float lightZ = (frameUBO.view * light.position).z;
       float lightNear = -lightZ - light.radius;
       float lightFar = -lightZ + light.radius;
       vec3 PL = light.position.xyz - worldPos;
  float dist = length(PL);
PL = normalize(PL);
    float attenuation = pow(max(0.0, 1.0 - pow(dist/light.radius, 4)), 2)
   / (dist * dist);
   float diffuse = max(0.0, dot(normal,PL));

    PLColor += light.color.xyz * diffuse * attenuation * light.intensity;
    };

   outColor = vec4(PLColor,1.0);

outColor = vec4(PLColor, 1.0);
}

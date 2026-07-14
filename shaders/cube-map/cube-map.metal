#include <metal_stdlib>
using namespace metal;


struct PushConstant {
    uint faceIndex;
    uint faceSize;
};


constant float PI = 3.14159265358979323846;


float2 sampleEquirectangular(float3 dir) {
    float elevation = asin(dir.y);
   float azimuth = atan2(dir.z, dir.x);

    float u = (azimuth + PI) / (2.0 * PI);
    float v =1.0 - (elevation + PI * 0.5) / PI;

   return float2(u,v);
}

float3 faceDirection(uint face, float2 uv) {
 float2 local = uv * 2.0 - 1.0;
 float3 dir;

switch(face) {
    case 0:
     dir= float3(1.0, -local.y, -local.x);
     break;
    case 1:
     dir= float3(-1.0, -local.y, local.x);
      break;
    case 2:
     dir= float3(local.x, 1.0, local.y);
      break;
    case 3:
     dir= float3(local.x, -1.0, -local.y);
      break;
    case 4:
     dir= float3(local.x, -local.y, 1.0);
      break;
    default:
     dir= float3(-local.x, -local.y, -1.0);
      break;
}

return normalize(dir);
}


kernel void comp(
    constant PushConstant& pc [[buffer(1)]],
    texture2d<float, access::sample> equirectangularSource [[texture(2)]],
    sampler texSamp [[sampler(0)]],

    texture2d<half, access::write> cubeFace [[texture(3)]],

     uint2 pixelCoord [[thread_position_in_grid]]
) {


  if(pixelCoord.x >= uint(pc.faceSize) || pixelCoord.y >= uint(pc.faceSize)) return;

  float2 uv = (float2(pixelCoord) + 0.5) / float(pc.faceSize);
  float3 dir = faceDirection(pc.faceIndex, uv);
  float2 samplerUV = sampleEquirectangular(dir);
   float3 color = equirectangularSource.sample(texSamp, samplerUV).rgb;
  cubeFace.write(
    half4(half3(color), 1.0h),
    pixelCoord
  );
}
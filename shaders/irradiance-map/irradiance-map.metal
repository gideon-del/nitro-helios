#include <metal_stdlib>
using namespace metal;


 struct PushConstant {
    uint faceIndex;
    uint faceSize;
};

constant float PI = 3.14159265358979323846;



float3 getNormalDirection(float2 uv, uint faceIndex) {
    float2 local = uv * 2.0 -1.0;

    float3 dir;

    switch(faceIndex) {
        case 0:
        dir = float3(1.0, -local.y, -local.x);
        break;
        case 1:
        dir = float3(-1.0, -local.y, local.x);
        break;
        case 2:
        dir = float3(local.x, 1.0, local.y);
        break;
        case 3:
        dir = float3(local.x, -1.0, -local.y);
        break;
        case 4:
        dir = float3(local.x, -local.y, 1.0);
        break;
        default:
        dir = float3(-local.x, -local.y, -1.0);
        break;
    }

    return normalize(dir);
}

float3x3 generateTBNMatrix( float3 N) {
float3 up =
    abs(N.y) < 0.999
    ? float3(0,1,0)
    : float3(1,0,0);
float3 right = normalize(cross(up, N));
up = normalize(cross(N,right));

return float3x3(right,up,N);

}


float3 sampleIrradiance(float3x3 TBN, texturecube<float> environment, sampler texSamp) {
 float3 irradiance = float3(0.0);
 uint sampleCount =0u;
 float sampleDelta = 0.001;

 for(float theta =0; theta <= PI/2; theta+= sampleDelta){
    for(float phi =0.0; phi <= 2*PI; phi += sampleDelta) {
 float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta)* sin(phi), cos(theta));
 float3 mainSample = TBN * tangentSample;

 irradiance += environment.sample(texSamp, mainSample, level(0.0)).rgb * cos(theta) * sin(theta);
 sampleCount += 1u;
 }
 }

 return PI* irradiance / float(sampleCount);

}


kernel void comp(
    constant PushConstant& pc [[buffer(1)]],
    texturecube<float, access::sample> envTexture [[texture(2)]],
    texture2d<half, access::write> cubeFace [[texture(3)]],
    sampler texSamp [[sampler(0)]],

    uint2 pixelCoord [[thread_position_in_grid]]
) {

     if(pixelCoord.x >= pc.faceSize || pixelCoord.y >= pc.faceSize ) return;

     float2 uv = (float2(pixelCoord) +  0.5) / float(pc.faceSize);
     float3 normal = getNormalDirection(uv, pc.faceIndex);
     float3x3 TBN = generateTBNMatrix(normal);
     float3 irradiance = sampleIrradiance(TBN, envTexture, texSamp);

     cubeFace.write(
        half4(half3(irradiance), 1.0h),
        pixelCoord
     );
}
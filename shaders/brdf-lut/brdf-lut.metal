#include <metal_stdlib>
using namespace metal;

struct  PushConstant {
    uint faceSize;
};
constant float PI = 3.14159265358979323846;

float3x3 generateTBNMatrix( float3 N) {
float3 up =
    abs(N.y) < 0.999
    ? float3(0,1,0)
    : float3(1,0,0);
float3 right = normalize(cross(up, N));
up = normalize(cross(N,right));

return float3x3(right,up,N);

}

float radicalInverse(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 hammersley(uint i, uint N) {
    return float2(float(i) / float(N), radicalInverse(i));
}

float3 ImportanceSampleGGX( float roughness, float2 Xi, float3x3 TBN) {

 float a = roughness*roughness;
    float phi = 2.0 * PI * Xi.x;

  float cosTheta =
sqrt(
    (1.0 - Xi.y) /
    (1.0 + (a*a - 1.0) * Xi.y)
); 

float sinTheta = sqrt( 1.0 - cosTheta*cosTheta);

float3 H;

H.x = sinTheta * cos(phi);
H.y = sinTheta * sin(phi);
H.z  = cosTheta;

H = normalize(TBN * H);

return H;

}


float geometrySmithIBL(float NdotV, float roughness) {
  float r = roughness;
  float k = (r*r)/2.0;
  return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(float3 N, float3 L, float3 V, float roughness) {
float ggx_V =geometrySmithIBL(max(dot(N,V), 0.0), roughness);
float ggx_L =geometrySmithIBL(max(dot(N,L), 0.0), roughness);

return ggx_L * ggx_V;
}


kernel void comp(
     constant PushConstant& pc [[buffer(1)]],
    texture2d<half, access::write> cubeFace [[texture(2)]],


    uint2 pixelCoord [[thread_position_in_grid]]
) {
  if(pixelCoord.x >= pc.faceSize || pixelCoord.y >= pc.faceSize){
        return;
    } 

    float2 uv = (float2(pixelCoord ) + 0.5) /float(pc.faceSize);
     float roughness = uv.y;
     float NdotV = max(uv.x, 0.0001);
    float3 N = float3(0.0,0.0,1.0);
    float3 V = float3(
       sqrt( 1.0 - NdotV*NdotV),
       0.0,
       NdotV
    );

    uint SAMPLES = 1024u;
   
   float A =0.0;
   float B = 0.0;
   float numSamples = 0.0;
   float3x3 TBN = float3x3(
    float3(1,0,0),
    float3(0,1,0),
    N
   );
   for(uint i =0u;i  < SAMPLES; i++) {
    float2 Xi = hammersley(i, SAMPLES);
    float3 H = ImportanceSampleGGX(roughness, Xi, TBN);
    float3 L = normalize(reflect(-V, H));

    float NdotL = max(dot(N, L), 0.0);
    if(NdotL <= 0.0){
        continue;
    }

    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float G = geometrySmith(N, L, V, roughness);

    float weight =(G * VdotH) /max(( NdotH * NdotV), 0.0001);
    float Fc = pow(1.0 - VdotH, 5.0);

    A += (1.0 - Fc) * weight;
    B += Fc * weight;
    numSamples++;
   }
    
 if(numSamples > 0.0){
     A /= numSamples;
  B /= numSamples;
 }


cubeFace.write(
    half4(A,B, 0.0h, 1.0h),
    pixelCoord
);
      
}
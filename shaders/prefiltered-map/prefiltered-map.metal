#include <metal_stdlib>
using namespace metal;


struct PushConstant {
    uint faceIndex;
    uint faceSize;
    float roughness;
    uint resolution;
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

float DistributionGGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;

    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;

    return a2 / (PI * denom * denom);
}


kernel void comp(
    constant PushConstant& pc [[buffer(1)]],

    texturecube<float, access::sample> envTexture [[texture(2)]],
    texture2d<half, access::write> cubeFace [[texture(3)]],
    sampler texSamp [[sampler(0)]],

    uint2 pixelCoord [[thread_position_in_grid]]
) {
    if(pixelCoord.x >= pc.faceSize || pixelCoord.y >= pc.faceSize){
        return;
    } 

float2 uv = (float2(pixelCoord) + 0.5) / float(pc.faceSize);
float3 N = getNormalDirection(uv, pc.faceIndex);
float3 V = N;
uint SAMPLES = 1024u;
float3x3 TBN = generateTBNMatrix(N);
float3 accumulation = float3(0.0);
float totalWeight = 0.0;
float roughness = pc.roughness;
for(uint i =0u; i <= SAMPLES; i++ ){
    float2 Xi = hammersley(i, SAMPLES);
    float3 H = ImportanceSampleGGX( roughness,Xi, TBN);
    float3 L = reflect(-V, H);

    float NdotL = max(dot(N,L), 0.0);
    if(NdotL > 0.0){
       float NdotH = max(dot(N,H),0.0);
    float VdotH = NdotH;
    float D = DistributionGGX(NdotH, roughness);

    float pdf =(D * NdotH / (4.0 * VdotH)) + 0.0001;
    float saTexel = 4.0 * PI / (6.0 * pc.resolution * pc.resolution); 
    float saSample = 1.0 / (float(SAMPLES) * pdf + 0.0001);
    float maxMip = log2(float(pc.resolution)); 
    float mipLevel = roughness == 0.0 ? 0.0 : 
    0.5 * log2(saSample / saTexel);

    accumulation += envTexture.sample(texSamp, L, level(clamp(mipLevel, 0.0, maxMip))).rgb * NdotL;
    totalWeight+=NdotL;
    }
}

if(totalWeight > 0.0) {
    accumulation /= totalWeight;
}

cubeFace.write(
    half4(
        half3(accumulation),1.0
    ),
    pixelCoord
);

}
#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};


vertex VertexOut vs(
    uint vertexID [[vertex_id]]
) {
    constexpr float2 positions[3] =
    {
        float2(-1.0,  3.0),
        float2( 3.0, -1.0),
        float2(-1.0, -1.0)
    };

     VertexOut out;

    out.position = float4(positions[vertexID], 0.0, 1.0);
    out.uv = (positions[vertexID] + 1.0) * 0.5;

    return out;
}


struct PushConstant {
    float exposure;
    uint mode;
};

float3 reinhard(float3 hdrColor) {
    return (hdrColor) /(hdrColor +1);
}

float3 acesApprox(float3 hdrColor) {
     float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((hdrColor * (a*hdrColor + b))/ (hdrColor * (c*hdrColor + d) + e), 0.0, 1.0);
}


float3 gammaCorrect(float3 color) {
    return pow(color, float3(1.0/2.2));
}

 fragment float4 fs(
    VertexOut in [[stage_in]],
    constant PushConstant& pc [[buffer(1)]],
    texture2d<float> hdrTex [[texture(2)]],
    sampler hdrSampler [[sampler(0)]]
 ) {
  
  float3 hdrColor = hdrTex.sample(hdrSampler, in.uv).rgb;

  hdrColor *= pc.exposure;
  
  float3 mappedColor;
  switch(pc.mode) {
    case 1:
     mappedColor = reinhard(hdrColor);
     break;
    case 2:
     mappedColor = acesApprox(hdrColor);
     break;
    default:
     mappedColor = clamp(hdrColor, 0.0, 1.0);
     break; 
  }

return float4(gammaCorrect(mappedColor),1.0);
  
 };
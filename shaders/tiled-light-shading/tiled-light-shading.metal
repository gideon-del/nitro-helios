#include <metal_stdlib>
using namespace metal;

struct PointLight
{
    float4 position;
    float4 color;
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

struct FrameUniformBuffer
{
    float4x4 invViewProj;
    float4x4 view;
    float2 screenSize;
    uint numTilesX;
    uint maxLightPerTile;
    uint showHeatMap;
};

struct VSOut
{
    float4 position [[position]];
    float2 uv;
};

float3 reconstructPosition(float2 uv,
                           float depth,
                           float4x4 invViewProj)
{
    float4 clipPos = float4(
        uv.x * 2.0 - 1.0,
        (1.0 - uv.y) * 2.0 - 1.0,
        depth,
        1.0);

    float4 worldPos = invViewProj * clipPos;

    return worldPos.xyz / worldPos.w;
}

float3 decodeNormal(float2 n)
{
    float2 f = n * 2.0 - 1.0;

    float3 v = float3(
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

vertex VSOut vs(uint vertexID [[vertex_id]])
{
    constexpr float2 positions[3] =
    {
        float2(-1.0,  3.0),
        float2( 3.0, -1.0),
        float2(-1.0, -1.0)
    };
    

    VSOut out;

    out.position = float4(positions[vertexID], 0.0, 1.0);
    out.uv = (positions[vertexID] + 1.0) * 0.5;

    return out;
}

fragment float4 fs(
    VSOut in [[stage_in]],

    constant FrameUniformBuffer& frameUBO [[buffer(2)]],

    texture2d<float> gDepth [[texture(3)]],
    texture2d<float> gNormal [[texture(4)]],
    sampler gSamp [[sampler(0)]],
    device const PointLight* pointLights [[buffer(5)]],
    device const uint* tileLightCount [[buffer(6)]],
    device const uint* tileLightIndices [[buffer(7)]],
    device const TileDebug* debugTiles [[buffer(8)]])
{
   

    float2 uv = in.position.xy / frameUBO.screenSize;

    float depth =
        gDepth.sample(gSamp, uv).r;

    float3 normal =
        decodeNormal(
            gNormal.sample(gSamp, uv).xy);

    float3 worldPos =
        reconstructPosition(
            uv,
            depth,
            frameUBO.invViewProj);


 
      uint2 tileId = uint2(in.position.xy) / 16;

    uint tile =
        tileId.y * frameUBO.numTilesX +
        tileId.x;

    uint lightCount =
        tileLightCount[tile];

    TileDebug tileInfo =
        debugTiles[tile];

    uint baseIdx =
        tile * frameUBO.maxLightPerTile;

    float3 PLColor = float3(0.0);

    if (frameUBO.showHeatMap == 1)
    {
        float t =
            clamp(float(lightCount) / 32.0,
                  0.0,
                  1.0);

        float3 color =
            mix(float3(0,0,1),
                float3(0,1,0),
                t);

        return float4(color,1.0);
    }

    for (uint i = 0; i < lightCount; ++i)
    {
        uint lightIdx =
            tileLightIndices[baseIdx + i];

        PointLight light =
            pointLights[lightIdx];

        

        float3 PL =
            light.position.xyz - worldPos;

        float dist =
            length(PL);

        PL = normalize(PL);

        float attenuation =
            pow(max(0.0,
                    1.0 - pow(dist / light.radius, 4.0)),
                2.0)
            / (dist * dist);

        float diffuse =
            max(0.0,
                dot(normal, PL));

        PLColor +=
            light.color.xyz *
            diffuse *
            attenuation *
            light.intensity;
    }

    return float4(PLColor, 1.0);
}
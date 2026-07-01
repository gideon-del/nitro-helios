#include <metal_stdlib>
using namespace metal;

struct DebugVertex
{
    float4 position;
    float4 color;
};

struct PushConstant
{
    float4x4 viewProj;
};

struct VSOut
{
    float4 position [[position]];
    float3 color;
};

vertex VSOut vs(
    uint vid [[vertex_id]],
    constant PushConstant& pc [[buffer(1)]],
    device const DebugVertex* debugVertices [[buffer(2)]])
{
    VSOut out;

    DebugVertex debugVertex = debugVertices[vid];

    out.position = pc.viewProj * debugVertex.position;
    out.color = debugVertex.color.xyz;

    return out;
}

fragment float4 fs(VSOut in [[stage_in]])
{
    return float4(in.color, 1.0);
}
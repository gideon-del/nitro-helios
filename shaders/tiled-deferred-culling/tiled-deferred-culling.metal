#include <metal_stdlib>
using namespace metal;
constant uint MAX_LIGHTS_PER_TILE = 256;



struct TileDebug
{
    uint lightCount;
    float minDepth;
    float maxDepth;
    float tileNear;
    float tileFar;
    uint overflow;
};

struct PointLight {
 float4 position;
 float4 color;
 float radius;
 float intensity;
};

struct Camera
{
    float4x4 view;
    float4x4 invProj;

    float2 screenSize;

    float nearPlane;
    float farPlane;

    uint numTilesX;
    uint numTilesY;

    uint totalLightCount;
};

struct TileFrustum {
    float3 topNormal;
    float3 bottomNormal;
    float3 leftNormal;
    float3 rightNormal;
    float tileNear;
    float tileFar;
};

float linearizeDepth(float depthNDC, float near, float far) {
return (near * far) / (far - depthNDC * (far - near));
}
bool lightOverlapTest(PointLight light, float tileNear, float tileFar, float4x4 cameraView) {
 float lightZ = (cameraView * light.position).z;
 float lightNear = -lightZ - light.radius;
 float lightFar = -lightZ + light.radius;

 return lightNear <= tileFar && lightFar >= tileNear;
}


float2 convertToNDC(float2 tile, float2 screenSize) {
  float2 uv = (tile * 16.0) / screenSize;
    return float2(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0);
}
float3 reconstructCameraViewProj(float2 tile, float2 screenSize, float4x4 invProj) {
    float4 clip =  invProj * float4(convertToNDC(float2(tile.x,tile.y), screenSize),1.0,1.0);
    return clip.xyz / clip.w;
}
TileFrustum calculateTileFrustum(float2 tile, float2 screenSize, float4x4 invProj,float tileNear, float tileFar) {

float3 topLeft = reconstructCameraViewProj(float2(tile.x,tile.y), screenSize, invProj);
float3 topRight = reconstructCameraViewProj(float2(tile.x+1,tile.y), screenSize, invProj);
float3 bottomRight = reconstructCameraViewProj(float2(tile.x+1,tile.y+1), screenSize, invProj);
float3 bottomLeft = reconstructCameraViewProj(float2(tile.x,tile.y+1), screenSize, invProj);

topLeft = normalize(topLeft - float3(0,0,0));
topRight = normalize(topRight - float3(0,0,0));
bottomRight = normalize(bottomRight - float3(0,0,0));
bottomLeft = normalize(bottomLeft - float3(0,0,0));

TileFrustum frustum;
frustum.topNormal = normalize(cross(topLeft, topRight));
frustum.rightNormal = normalize(cross(topRight, bottomRight));
frustum.bottomNormal = normalize(cross(bottomRight,bottomLeft));
frustum.leftNormal = normalize(cross(bottomLeft,topLeft));
frustum.tileFar = tileFar;
frustum.tileNear = tileNear;

return frustum;

}

bool isLightInTileFrustum(PointLight light, float4x4 cameraView,float3 topNormal, float3 bottomNormal, float3 leftNormal, float3 rightNormal) {
float3 lightViewPos = (cameraView * light.position).xyz;


bool isInTop = dot(topNormal, lightViewPos) + light.radius >= 0.0;
bool isInBottom = dot(bottomNormal,lightViewPos) + light.radius >= 0.0;
bool isInRight = dot(rightNormal, lightViewPos) + light.radius >= 0.0;
bool isInLeft =  dot(leftNormal, lightViewPos) + light.radius >= 0.0;

return isInBottom && isInTop && isInRight && isInLeft;
}
kernel void comp(
    constant Camera& camera [[buffer(2)]],
    texture2d<float> depthTexture [[texture(3)]],
    device PointLight* pointLights [[buffer(4)]],
    device uint* tileLightCount [[buffer(5)]],
    device uint* tileLightIndices [[buffer(6)]],
    device TileDebug* debugTiles [[buffer(7)]],

    uint3 gid [[thread_position_in_grid]],
    uint3 groupID [[threadgroup_position_in_grid]],
    uint3 localID [[thread_position_in_threadgroup]],
    uint localIndex [[thread_index_in_threadgroup]])
{
     threadgroup atomic_uint minDepthInt;
 threadgroup atomic_uint maxDepthInt;
 threadgroup atomic_uint lightCount;

  threadgroup uint lightIndices[MAX_LIGHTS_PER_TILE];

 threadgroup float3 sharedTopNormal;
 threadgroup float3 sharedBottomNormal;
 threadgroup float3 sharedLeftNormal;
 threadgroup float3 sharedRightNormal;
    uint2 tileId = groupID.xy;
bool inBounds = gid.x < uint(camera.screenSize.x) &&
                gid.y < uint(camera.screenSize.y);

    if (localIndex == 0)
    {
        atomic_store_explicit(&minDepthInt, 0xffffffffu, memory_order_relaxed);
        atomic_store_explicit(&maxDepthInt, 0u, memory_order_relaxed);
        atomic_store_explicit(&lightCount, 0u, memory_order_relaxed);
    }

    threadgroup_barrier(mem_flags::mem_threadgroup);

 if(inBounds){
       float depth = depthTexture.read(gid.xy).r;

    atomic_fetch_min_explicit(
        &minDepthInt,
        as_type<uint>(depth),
        memory_order_relaxed);

    atomic_fetch_max_explicit(
        &maxDepthInt,
        as_type<uint>(depth),
        memory_order_relaxed);
 }

    threadgroup_barrier(mem_flags::mem_threadgroup);

    float minDepth =
        as_type<float>(
            atomic_load_explicit(
                &minDepthInt,
                memory_order_relaxed));

    float maxDepth =
        as_type<float>(
            atomic_load_explicit(
                &maxDepthInt,
                memory_order_relaxed));

    float tileNear =
        linearizeDepth(
            minDepth,
            camera.nearPlane,
            camera.farPlane);

    float tileFar =
        linearizeDepth(
            maxDepth,
            camera.nearPlane,
            camera.farPlane);

    constexpr uint groupSizeX = 16u;
    constexpr uint groupSizeY = 16u;
    constexpr uint totalThreads = groupSizeX * groupSizeY;

    uint totalLightCount = camera.totalLightCount;

    uint lightsPerThread =
        uint(ceil(float(totalLightCount) / float(totalThreads)));

    if (localIndex == 0)
    {
        TileFrustum tileFrustum =
            calculateTileFrustum(
                float2(tileId),
                camera.screenSize,
                camera.invProj,
                tileNear,
                tileFar);

        sharedTopNormal = tileFrustum.topNormal;
        sharedBottomNormal = tileFrustum.bottomNormal;
        sharedLeftNormal = tileFrustum.leftNormal;
        sharedRightNormal = tileFrustum.rightNormal;
    }

    threadgroup_barrier(mem_flags::mem_threadgroup);

    for (uint n = 0; n < lightsPerThread; n++)
    {
        uint lightIdx =
            localIndex + n * totalThreads;

        if (lightIdx >= totalLightCount)
            continue;

        bool passesDepth =
            lightOverlapTest(
                pointLights[lightIdx],
                tileNear,
                tileFar,
                camera.view);

        bool passesFrustum =
            isLightInTileFrustum(
                pointLights[lightIdx],
                camera.view,
                sharedTopNormal,
                sharedBottomNormal,
                sharedLeftNormal,
                sharedRightNormal);

        if (passesDepth && passesFrustum)
        {
            uint slot =
                atomic_fetch_add_explicit(
                    &lightCount,
                    1,
                    memory_order_relaxed);

            if (slot < MAX_LIGHTS_PER_TILE)
            {
                lightIndices[slot] = lightIdx;
            }
        }
    }

    threadgroup_barrier(mem_flags::mem_threadgroup);

    if (localIndex == 0)
    {
        uint tile =
            tileId.y * camera.numTilesX +
            tileId.x;

        uint base =
            tile * MAX_LIGHTS_PER_TILE;

        uint count =
            atomic_load_explicit(
                &lightCount,
                memory_order_relaxed);

        uint clampedCount =
            min(count, uint(MAX_LIGHTS_PER_TILE));

        tileLightCount[tile] = clampedCount;
float lz = (camera.view * pointLights[0].position).z;
        debugTiles[tile].lightCount = count;
        debugTiles[tile].tileNear = camera.numTilesX;
        debugTiles[tile].tileFar = tileFar;
        debugTiles[tile].minDepth = minDepth;
        debugTiles[tile].maxDepth = maxDepth;
        debugTiles[tile].overflow = count - clampedCount;

        for (uint i = 0; i < clampedCount; i++)
        {
            tileLightIndices[base + i] =
                lightIndices[i];
        }
    }
}
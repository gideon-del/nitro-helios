#pragma once
#include <math/math.h>
#include <vector>

using namespace nitro::math;

namespace nitro::rasterizer
{
    struct VertexOut
    {
        Vec3D worldPos;
        Vec3D screenPos;
        Vec2D uv;
        Vec3D normal;
        float w;
    };
    struct VertexIn
    {
        Vec3D position;
        Vec2D uv;
        Vec3D normal;
    };
    struct Triangle
    {
        VertexIn v0, v1, v2;
    };
    VertexOut processVertex(
        const VertexIn &v,
        const Mat4 &model,
        const Mat4 &view,
        const Mat4 &projection,
        int width, int height);
    std::vector<Triangle> processOBJFile(std::string fileName);
}
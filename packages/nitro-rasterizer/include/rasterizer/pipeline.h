#pragma once
#include <math/math.h>
#include <vector>

using namespace nitro::math;

namespace nitro::rasterizer
{
    struct VertexOut
    {
        Vec3D position;
        Vec2D uv;
        Vec3D normal;
        float w;
    };
    struct Vertex
    {
        Vec3D position;
        Vec2D uv;
        Vec3D normal;
    };
    struct Triangle
    {
        Vertex v0, v1, v2;
    };
    Vec4D processVertex(
        const Vec4D &v,
        const Mat4 &model,
        const Mat4 &view,
        const Mat4 &projection,
        int width, int height);
    std::vector<Triangle> processOBJFile(std::string fileName);
}
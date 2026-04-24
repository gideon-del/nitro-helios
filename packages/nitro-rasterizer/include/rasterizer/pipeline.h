#pragma once
#include <math/math.h>
#include <vector>

using namespace nitro::math;

namespace nitro::rasterizer
{

    enum class ClipPlane
    {
        Far,
        Near,
        Left,
        Right,
        Top,
        Bottom
    };
    struct VertexClipIn
    {
        Vec4D viewPos;
        Vec4D clipPos;
        Vec3D normalVS;
        Vec2D uv;
    };

    struct VertexOut
    {

        Vec3D screenPos;
        Vec2D uv;
        Vec3D normal;
        Vec3D pos_over_w;
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
        VertexIn a, b, c;
    };

    struct ClippedTriangle
    {
        VertexClipIn a, b, c;
    };
    bool isInsidePlane(const Vec4D &a, ClipPlane plane);
    VertexClipIn findIntersectionBetweenVertex(const VertexClipIn &a, const VertexClipIn &b, float near);
    VertexClipIn processVertexClip(
        const VertexIn &v,
        const Mat4 &model,
        const Mat4 &view,
        const Mat4 &proj);

    std::vector<VertexClipIn> filterVertexByPlane(const std::vector<VertexClipIn> &v, const ClipPlane &plane);
    float findVertexDistanceByPlane(const VertexClipIn &a, const ClipPlane &plane);

    std::vector<ClippedTriangle> clipPolygons(const std::vector<VertexClipIn> &v);

    VertexOut processScreenVertex(
        const VertexClipIn &v,
        int width, int height);

    std::vector<Triangle> processOBJFile(std::string fileName);
}
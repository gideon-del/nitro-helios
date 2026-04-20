#pragma once
#include <math/math.h>

using namespace nitro::math;

namespace nitro::rasterizer
{
    struct VertexOut
    {
        // Vec4D clip;
        // Vec3D ndc;
        Vec3D screen;
        float z;
        float w;
    };

    VertexOut processVertex(
        const Vec4D &v,
        const Mat4 &model,
        const Mat4 &view,
        const Mat4 &projection,
        int width, int height);
}
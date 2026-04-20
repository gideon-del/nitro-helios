#pragma once
#include <math/math.h>
#include <rasterizer/framebuffer.h>
#include <rasterizer/pipeline.h>
#include <rasterizer/texture.h>
using namespace nitro::math;
namespace nitro::rasterizer
{

    float edgeFn(const Vec2D &a, const Vec2D &b, const Vec2D &p);
    bool isInsideTriangle(const Vec3D &a, const Vec3D &b, const Vec3D &c, const Vec3D &p);
    void drawTriangleEdge(FrameBuffer &fb, VertexOut a, VertexOut b, VertexOut c, const Texture &texture);
    float perspectiveCorrection(float a0, float a1, float a2, float aw1, float aw2, float aw3, float w0, float w1, float w2, float w3);
}
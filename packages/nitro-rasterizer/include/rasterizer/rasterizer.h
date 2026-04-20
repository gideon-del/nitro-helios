#pragma once
#include <math/math.h>
#include <rasterizer/framebuffer.h>
#include <rasterizer/pipeline.h>
using namespace nitro::math;
namespace nitro::rasterizer
{

    float edgeFn(const Vec2D &a, const Vec2D &b, const Vec2D &p);
    bool isInsideTriangle(const Vec3D &a, const Vec3D &b, const Vec3D &c, const Vec3D &p);
    void drawTriangleEdge(FrameBuffer &fb, VertexOut a, VertexOut b, VertexOut c, uint8_t red, uint8_t green, uint8_t blue);
}
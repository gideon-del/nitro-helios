#pragma once
#include <math/math.h>
#include <rasterizer/framebuffer.h>
#include <rasterizer/pipeline.h>
#include <rasterizer/texture.h>
using namespace nitro::math;
namespace nitro::rasterizer
{

    float edgeFn(const Vec3D &a, const Vec3D &b, const Vec3D &p);
    bool isInsideTriangle(const Vec3D &a, const Vec3D &b, const Vec3D &c, const Vec3D &p);
    void drawTriangleEdge(FrameBuffer &fb, VertexOut a, VertexOut b, VertexOut c, const Texture &texture, const Vec3D &lightPos, const Vec3D &cameraPos);

}
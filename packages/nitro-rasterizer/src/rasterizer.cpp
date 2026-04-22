#include <rasterizer/rasterizer.h>
#include <rasterizer/texture.h>
#include <rasterizer/color.h>

using namespace nitro::math;

namespace nitro::rasterizer
{
    float edgeFn(const Vec3D &a, const Vec3D &b, const Vec3D &p)
    {
        return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
    }
    bool isInsideTriangle(const Vec3D &a, const Vec3D &b, const Vec3D &c, const Vec3D &p)
    {
        float E0 = edgeFn(a, b, p);
        float E1 = edgeFn(b, c, p);
        float E2 = edgeFn(c, a, p);

        return (E0 >= 0 && E1 >= 0 && E2 >= 0) || (E0 <= 0 && E1 <= 0 && E2 <= 0);
    }

    void drawTriangleEdge(FrameBuffer &fb, VertexOut a, VertexOut b, VertexOut c, const Texture &texture)
    {
        float area = edgeFn(a.position, b.position, c.position);

        if (area < 0)
        {
            std::swap(b, c);
            area = edgeFn(a.position, b.position, c.position);
        }

        int minX = (int)std::floor(std::min({a.position.x, b.position.x, c.position.x}));
        int maxX = (int)std::ceil(std::max({a.position.x, b.position.x, c.position.x}));

        int minY = (int)std::floor(std::min({a.position.y, b.position.y, c.position.y}));
        int maxY = (int)std::ceil(std::max({a.position.y, b.position.y, c.position.y}));

        for (int y = minY; y <= maxY; y++)
        {
            for (int x = minX; x <= maxX; x++)
            {
                Vec3D p = Vec3D{x + 0.5f, y + 0.5f, 1};
                float w0 = edgeFn(b.position, c.position, p) / area;
                float w1 = edgeFn(c.position, a.position, p) / area;
                float w2 = edgeFn(a.position, b.position, p) / area;

                if (w0 >= 0 && w1 >= 0 && w2 >= 0)
                {
                    float denom = (w0 / a.w) + (w1 / b.w) + (w2 / c.w);

                    float z = ((w0 * a.position.z / a.w) + (w1 * b.position.z / b.w) + (w2 * c.position.z / c.w)) / denom;
                    float u = ((w0 * a.uv.x / a.w) + (w1 * b.uv.x / b.w) + (w2 * c.uv.x / c.w)) / denom;
                    float v = ((w0 * a.uv.y / a.w) + (w1 * b.uv.y / b.w) + (w2 * c.uv.y / c.w)) / denom;

                    if (fb.testAndSetDepth(x, y, z))
                    {

                        Color col = texture.sample(u, v);

                        fb.setPixel(x, y, col);
                    }
                }
            }
        }
    }

}
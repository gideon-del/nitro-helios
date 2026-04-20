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
        float area = edgeFn(a.screen, b.screen, c.screen);

        if (area < 0)
        {
            std::swap(b, c);
            area = edgeFn(a.screen, b.screen, c.screen);
        }

        int minX = (int)std::floor(std::min({a.screen.x, b.screen.x, c.screen.x}));
        int maxX = (int)std::ceil(std::max({a.screen.x, b.screen.x, c.screen.x}));

        int minY = (int)std::floor(std::min({a.screen.y, b.screen.y, c.screen.y}));
        int maxY = (int)std::ceil(std::max({a.screen.y, b.screen.y, c.screen.y}));

        for (int y = minY; y <= maxY; y++)
        {
            for (int x = minX; x <= maxX; x++)
            {
                Vec3D p = Vec3D{x + 0.5f, y + 0.5f, 1};
                float w0 = edgeFn(b.screen, c.screen, p) / area;
                float w1 = edgeFn(c.screen, a.screen, p) / area;
                float w2 = edgeFn(a.screen, b.screen, p) / area;

                if (w0 >= 0 && w1 >= 0 && w2 >= 0)
                {
                    float denom = (w0 / a.w) + (w1 / b.w) + (w2 / c.w);

                    float z = ((w0 * a.z / a.w) + (w1 * b.z / b.w) + (w2 * c.z / c.w)) / denom;
                    float u = ((w0 * a.u / a.w) + (w1 * b.u / b.w) + (w2 * c.u / c.w)) / denom;
                    float v = ((w0 * a.v / a.w) + (w1 * b.v / b.w) + (w2 * c.v / c.w)) / denom;
                    if (fb.testAndSetDepth(x, y, z))
                    {
                        Color col = texture.sampleBilinear(u, v);

                        fb.setPixel(x, y, col);
                    }
                }
            }
        }
    }

}
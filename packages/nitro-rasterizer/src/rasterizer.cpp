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

    void drawTriangleEdge(FrameBuffer &fb, VertexOut a, VertexOut b, VertexOut c, const Texture &texture, const Vec3D &lightPos, const Vec3D &cameraPos)
    {
        float area = edgeFn(a.screenPos, b.screenPos, c.screenPos);
        Vec3D lightColor = {1, 1, 1};

        float Ka = 0.1f;
        float Kd = 1.0f;
        float Ks = 0.5f;
        float shininess = 32.0f;
        if (area <= 0)
        {
            return;
            // std::swap(b, c);
            // area = edgeFn(a.screenPos, b.screenPos, c.screenPos);
        }
        int minX = (int)std::floor(std::min({a.screenPos.x, b.screenPos.x, c.screenPos.x}));
        int maxX = (int)std::ceil(std::max({a.screenPos.x, b.screenPos.x, c.screenPos.x}));

        int minY = (int)std::floor(std::min({a.screenPos.y, b.screenPos.y, c.screenPos.y}));
        int maxY = (int)std::ceil(std::max({a.screenPos.y, b.screenPos.y, c.screenPos.y}));

        for (int y = minY; y <= maxY; y++)
        {
            for (int x = minX; x <= maxX; x++)
            {
                Vec3D p = Vec3D{x + 0.5f, y + 0.5f, 1};
                float w0 = edgeFn(b.screenPos, c.screenPos, p) / area;
                float w1 = edgeFn(c.screenPos, a.screenPos, p) / area;
                float w2 = edgeFn(a.screenPos, b.screenPos, p) / area;

                if (w0 >= 0 && w1 >= 0 && w2 >= 0)
                {
                    float denom = (w0 / a.w) + (w1 / b.w) + (w2 / c.w);

                    float z = ((w0 * a.screenPos.z / a.w) + (w1 * b.screenPos.z / b.w) + (w2 * c.screenPos.z / c.w)) / denom;

                    float u = ((w0 * a.uv.x / a.w) + (w1 * b.uv.x / b.w) + (w2 * c.uv.x / c.w)) / denom;
                    float v = ((w0 * a.uv.y / a.w) + (w1 * b.uv.y / b.w) + (w2 * c.uv.y / c.w)) / denom;

                    Vec3D P = ((((a.pos_over_w) * w0) + ((b.pos_over_w) * w1) + ((c.pos_over_w) * w2)) / denom);

                    Vec3D N = ((((a.normal / a.w) * w0) + ((b.normal / b.w) * w1) + ((c.normal / c.w) * w2)) / denom).normalize();

                    if (fb.testAndSetDepth(x, y, z))
                    {

                        Vec3D L = (lightPos - P).normalize();
                        Vec3D V = (cameraPos - P).normalize();
                        Vec3D H = (L + V).normalize();
                        float diffuse = std::max(0.0f, N.dot(L));
                        float specular = std::pow(std::max(0.0f, N.dot(H)), shininess);
                        Vec3D ambient = lightColor * Ka;

                        Color col = texture.sampleBilinear(u, v);
                        Vec3D texColor = col.toVec3();

                        Vec3D finalColor =
                            (texColor * (ambient +
                                         (lightColor * Kd * diffuse))) +
                            (lightColor * Ks * specular);

                        ;

                        fb.setPixel(x, y, Color::fromVec3(finalColor));
                    }
                }
            }
        }
    }

}
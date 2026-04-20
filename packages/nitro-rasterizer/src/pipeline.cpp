#include <rasterizer/pipeline.h>

namespace nitro::rasterizer
{
    VertexOut processVertex(const Vec4D &v,
                            const Mat4 &model,
                            const Mat4 &view,
                            const Mat4 &projection,
                            int width, int height)
    {

        // Get Clip
        VertexOut out;
        Vec4D clip = projection * view * model * v;

        out.w = clip.w;
        float invW = 1.0f / clip.w;

        Vec3D ndc = clip.perspectiveDivide();

        out.z = ndc.z;
        Vec3D screen;
        screen.x = (ndc.x + 1) * 0.5 * width;
        screen.y = (1 - ndc.y) * 0.5 * height;
        out.screen = screen;
        return out;
    }
}
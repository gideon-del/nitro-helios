#include <rasterizer/framebuffer.h>
#include <rasterizer/pipeline.h>
#include <rasterizer/rasterizer.h>
#include <rasterizer/texture.h>
#include <math/vec.h>
using namespace nitro::math;
using namespace nitro::rasterizer;

void drawObjModel(FrameBuffer &fb, float angle, const nitro::rasterizer::Texture &tex, std::vector<Triangle> &triangles)
{
    Vec3D lightPos = {2, 8, 7};
    Vec3D cameraPos = {0, 0, 5};
    Mat4 model = Quat::fromAxisAngle(Vec3D{3, 3, 2}, toRadians(angle)).toMat4();
    Mat4 view = Mat4::lookAt(cameraPos, Vec3D{0, 0, 0}, Vec3D{0, 1, 0});
    Mat4 proj = Mat4::perspective(toRadians(60), (float)fb.width / (float)fb.height, 0.1, 100);

    for (auto &triangle : triangles)
    {

        VertexClipIn aClip = processVertexClip(triangle.a, model, view, proj);
        VertexClipIn bClip = processVertexClip(triangle.b, model, view, proj);
        VertexClipIn cClip = processVertexClip(triangle.c, model, view, proj);

        std::vector<ClippedTriangle> clippedTriangles = clipPolygons({aClip, bClip, cClip});

        for (auto &clippedTri : clippedTriangles)
        {

            VertexOut aOut = processScreenVertex(clippedTri.a, fb.width, fb.height);
            VertexOut bOut = processScreenVertex(clippedTri.b, fb.width, fb.height);
            VertexOut cOut = processScreenVertex(clippedTri.c, fb.width, fb.height);

            // Backface Culling
            float area = edgeFn(aOut.screenPos, bOut.screenPos, cOut.screenPos);

            if (area > 0)
            {
                drawTriangleEdge(fb, aOut, bOut, cOut, tex, lightPos, cameraPos);
            }
        }
    }
}
int main()
{

    nitro::rasterizer::Texture tex = nitro::rasterizer::Texture::loadFromFilePath("texture.jpg");
    std::vector<Triangle> triangles = processOBJFile("suzanne.obj");

    for (int frame = 0; frame < 36; frame++)
    {
        FrameBuffer fb(1024, 1024);
        fb.clear(0, 0, 0, 255);

        drawObjModel(fb, frame * 10.0f, tex, triangles);

        std::string filename = "frame_" + std::to_string(frame) + ".ppm";
        fb.writePPM(filename);
    }

    return 0;
}
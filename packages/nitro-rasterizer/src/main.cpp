#include <rasterizer/framebuffer.h>
#include <rasterizer/pipeline.h>
#include <rasterizer/rasterizer.h>
#include <rasterizer/texture.h>
#include <math/vec.h>
using namespace nitro::math;
using namespace nitro::rasterizer;

void drawObjModel(FrameBuffer &fb, float angle, const nitro::rasterizer::Texture &tex, std::vector<Triangle> &triangles)
{
    Mat4 model = Mat4() * Quat::fromAxisAngle(Vec3D(0, 0, 3), toRadians(angle)).toMat4();
    Mat4 view = Mat4::lookAt(Vec3D{0, 0, 3}, Vec3D{0, 0, 0}, Vec3D{0, 1, 0});
    Mat4 proj = Mat4::perspective(toRadians(60), (float)fb.width / (float)fb.height, 0.1, 100);

    for (auto &triangle : triangles)
    {
        std::cout << "Drawing Triangle" << "\n";
        try
        {

            auto positionA = processVertex(Vec4D{triangle.v0.position.x, triangle.v0.position.y, triangle.v0.position.z, 1}, model, view, proj, fb.width, fb.height);
            auto positionB = processVertex(Vec4D{triangle.v1.position.x, triangle.v1.position.y, triangle.v1.position.z, 1}, model, view, proj, fb.width, fb.height);
            auto positionC = processVertex(Vec4D{triangle.v2.position.x, triangle.v2.position.y, triangle.v2.position.z, 1}, model, view, proj, fb.width, fb.height);

            VertexOut a = {
                positionA.toVec3D(),
                triangle.v0.uv,
                triangle.v0.normal,
                positionA.w};
            VertexOut b = {
                positionB.toVec3D(),
                triangle.v1.uv,
                triangle.v1.normal,
                positionB.w};
            VertexOut c = {
                positionC.toVec3D(),
                triangle.v2.uv,
                triangle.v2.normal,
                positionC.w};
            drawTriangleEdge(fb, a, b, c, tex);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    std::cout << "Drawing Triangle Done" << "\n";
}
int main()
{
    // black background
    // fb.drawLine(0, 0, 400, 400, 0, 255, 255);
    // fb.drawLine(0, 200, 400, 200, 0, 255, 255);
    // fb.drawLine(0, 200, 400, 0, 0, 255, 255);
    // Vec3D a = Vec3D{200, 200, 0.5};
    // Vec3D b = Vec3D{0, 400, 0.5};
    // Vec3D c = Vec3D{400, 400, 0.5};
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
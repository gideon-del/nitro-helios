#include <rasterizer/framebuffer.h>
#include <rasterizer/pipeline.h>
#include <rasterizer/rasterizer.h>
#include <rasterizer/texture.h>
#include <math/vec.h>
using namespace nitro::math;
using namespace nitro::rasterizer;
void draw3DCube(FrameBuffer &fb, float angle)
{
    Vec4D vertices[8] = {
        {-1, -1, -1, 1},
        {1, -1, -1, 1},
        {1, 1, -1, 1},
        {-1, 1, -1, 1},
        {-1, -1, 1, 1},
        {1, -1, 1, 1},
        {1, 1, 1, 1},
        {-1, 1, 1, 1},
    };
    Mat4 model = Mat4() * Quat::fromAxisAngle(Vec3D(0, 10, 20), toRadians(angle)).toMat4();
    // Mat4 model = Mat4();
    Mat4 view = Mat4::lookAt(Vec3D{2, 2, 4}, Vec3D{0, 0, 0}, Vec3D{0, 1, 0});
    Mat4 proj = Mat4::perspective(toRadians(120), (float)fb.width / (float)fb.height, 0.1, 100);
    nitro::rasterizer::Texture tex = nitro::rasterizer::Texture::loadFromFilePath("texture.jpg");
    int edges[12][2] = {
        // back face
        {0, 1},
        {1, 2},
        {2, 3},
        {3, 0},
        // front face
        {4, 5},
        {5, 6},
        {6, 7},
        {7, 4},
        // connecting edges
        {0, 4},
        {1, 5},
        {2, 6},
        {3, 7},
    };

    int faces[6][4] = {
        {0, 1, 2, 3}, // back
        {4, 5, 6, 7}, // front
        {0, 1, 5, 4}, // bottom
        {2, 3, 7, 6}, // top
        {0, 3, 7, 4}, // left
        {1, 2, 6, 5}, // right
    };
    // fb.cl
    VertexOut screen[8];
    for (int i = 0; i < 8; i++)
    {
        VertexOut vOut = processVertex(vertices[i], model, view, proj, fb.width, fb.height);
        screen[i] = vOut;
    }
    Color colors[6] = {
        Color{255, 0, 0, 255},   // red
        Color{0, 255, 0, 255},   // green
        Color{0, 0, 255, 255},   // blue
        Color{255, 255, 0, 255}, // yellow
        Color{255, 0, 255, 255}, // magenta
        Color{0, 255, 255, 255}, // cyan
    };

    for (int i = 0; i < 6; i++)
    {

        // Points
        VertexOut a = screen[faces[i][0]];
        VertexOut b = screen[faces[i][1]];
        VertexOut c = screen[faces[i][2]];
        VertexOut d = screen[faces[i][3]];
        a.u = 0.0f;
        a.v = 0.0f;
        b.u = 1.0f;
        b.v = 0.0f;
        c.u = 1.0f;
        c.v = 1.0f;
        d.u = 0.0f;
        d.v = 1.0f;
        drawTriangleEdge(fb, a, b, c, tex);
        drawTriangleEdge(fb, a, c, d, tex);
    }
    // fb.clearDepth();

    // for (auto &e : edges)
    //     fb.drawLine(screen[e[0]].screen.x, screen[e[0]].screen.y,
    //                 screen[e[1]].screen.x, screen[e[1]].screen.y,
    //                 Color{0, 255, 0, 255});
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

    for (int frame = 0; frame < 36; frame++)
    {
        FrameBuffer fb(400, 400);
        fb.clear(0, 0, 0, 255);

        draw3DCube(fb, frame * 10.0f);

        std::string filename = "frame_" + std::to_string(frame) + ".ppm";
        fb.writePPM(filename);
    }
    return 0;
}
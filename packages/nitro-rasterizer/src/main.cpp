#include "../includes/rasterizer/framebuffer.h"
#include <math/vec.h>
using namespace nitro::math;
int main()
{
    nitro::rasterizer::FrameBuffer fb(400, 400);
    fb.clear(0, 0, 0, 0); // black background
    fb.drawLine(0, 0, 400, 400, 0, 255, 255);
    // fb.drawLine(0, 200, 400, 200, 0, 255, 255);
    fb.drawLine(0, 200, 400, 0, 0, 255, 255);
    fb.drawTriangle(Vec2D{200, 200}, Vec2D{0, 400}, Vec2D{400, 400}, 255, 0, 0);
    fb.writePPM("output.ppm");
    return 0;
}
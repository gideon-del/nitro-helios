#pragma once
#include <iostream>
#include <math/math.h>

using namespace nitro::math;
namespace nitro::rasterizer
{
    struct Color
    {
        uint8_t r, g, b, a;
        Color() : r(0), g(0), b(0), a(255) {};
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {};
        Vec3D toVec3()
        {
            return {r / 255.0f, g / 255.0f, b / 255.0f};
        }
        static Color lerp(Color a, Color b, float t)
        {
            return {
                (uint8_t)(a.r + (b.r - a.r) * t),
                (uint8_t)(a.g + (b.g - a.g) * t),
                (uint8_t)(a.b + (b.b - a.b) * t),
                (uint8_t)(a.a + (b.a - a.a) * t),

            };
        }
        static Color fromVec3(const Vec3D &v)
        {
            return {(uint8_t)(v.x * 255.0f), (uint8_t)(v.y * 255.0f), (uint8_t)(v.z * 255.0f), 255};
        }
    };
}
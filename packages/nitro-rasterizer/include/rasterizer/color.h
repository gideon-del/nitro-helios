#pragma once
#include <iostream>
namespace nitro::rasterizer
{
    struct Color
    {
        uint8_t r, g, b, a;

        static Color lerp(Color a, Color b, float t)
        {
            return {
                (uint8_t)(a.r + (b.r - a.r) * t),
                (uint8_t)(a.g + (b.g - a.g) * t),
                (uint8_t)(a.b + (b.b - a.b) * t),
                (uint8_t)(a.a + (b.a - a.a) * t),

            };
        }
    };
}
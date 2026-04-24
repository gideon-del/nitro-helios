#pragma once
#include <cmath>
#include <algorithm>
#include <iostream>

namespace nitro::math
{
    inline float lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }
    inline float clamp(float v, float min, float max)
    {
        return std::min(max, std::max(v, min));
    }

    inline float clamp01(float v)
    {
        return clamp(v, 0.0f, 1.0f);
    }

    inline float smoothstep(float edge0, float edge1, float x)
    {
        float t = clamp01((x - edge0) / (edge1 - edge0));
        return t * t * (3.0f - 2.0f * t);
    }

    inline float smootherstep(float edge0, float edge1, float x)
    {
        float t = clamp01((x - edge0) / (edge1 - edge0));
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    inline float remap(float v, float inMin, float inMax, float outMin, float outMax)
    {
        return outMin + (outMax - outMin) * ((v - inMin) / (inMax - inMin));
    }

    inline float sign(float v)
    {
        return (v > 0.0f) ? 1.0f : (v < 0.0f) ? -1.0f
                                              : 0.0f;
    }

    inline float toRadians(float degrees) { return degrees * M_PI / 180.0f; }
    inline float toDegrees(float radians) { return radians * 180.0f / M_PI; }

    constexpr float PI = M_PI;
    constexpr float TAU = 2.0f * M_PI;
    constexpr float HALF_PI = PI / 2.0f;
    constexpr float EPSILON = 1e-6f;
}
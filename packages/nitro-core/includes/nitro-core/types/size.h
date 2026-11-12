#pragma once
#include <cstdint>

namespace nitro
{
    struct Extent2D
    {
        uint32_t width = 1, height = 1;

        bool operator==(const Extent2D &) const = default;
    };
    struct Extent3D
    {
        uint32_t width = 1, height = 1, depth = 1;

        bool operator==(const Extent3D &) const = default;
    };
    struct Offset2D
    {
        int32_t x = 0, y = 0;

        bool operator==(const Offset2D &) const = default;
    };
    struct Offset3D
    {
        int32_t x = 0, y = 0, z = 0;

        bool operator==(const Offset3D &) const = default;
    };
} // namespace nitro

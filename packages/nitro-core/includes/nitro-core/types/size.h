#pragma once
#include <cstdint>

namespace nitro
{
    struct Extent2D
    {
        uint32_t width = 1, height = 1;

        bool operator==(const Extent2D &o) const
        {
            return o.width == width && o.height == height;
        }

        bool operator!=(const Extent2D &o) const
        {
            return !(*this == o);
        }
    };
} // namespace nitro

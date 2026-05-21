#pragma once
#include <cstdint>

namespace nitro::rhi
{
    class RHISurface
    {
    public:
        virtual ~RHISurface() = default;
        virtual uint32_t getWidth() const = 0;
        virtual uint32_t getHeight() const = 0;
    };
}

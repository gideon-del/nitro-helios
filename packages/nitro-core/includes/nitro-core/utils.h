#pragma once
#include <cstdint>

namespace nitro
{
    inline uint64_t alignUp(uint64_t value, uint64_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }
} // namespace nitro

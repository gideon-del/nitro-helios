#pragma once
#include <cstdint>

namespace nitro
{
    inline uint64_t alignUp(uint64_t value, uint64_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    inline void hashCombine(size_t &seed, size_t h)
    {
        seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template <class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };

    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;
} // namespace nitro

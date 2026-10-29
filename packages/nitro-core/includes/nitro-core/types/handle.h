#pragma once
#include <cstdint>
#include <limits>
#include "unordered_map"
namespace nitro
{

    using HandleValueType = uint32_t;
    inline constexpr HandleValueType INVALID_HANDLE_ID = std::numeric_limits<HandleValueType>::max();
    ;
    template <typename T>
    struct Handle
    {

        HandleValueType index = INVALID_HANDLE_ID;
        HandleValueType generation = 0;

        bool operator==(const Handle<T> &o) const
        {
            return o.index == index && generation == o.generation;
        }
        bool isValid() const
        {
            return index != INVALID_HANDLE_ID;
        }
    };

    template <typename T>
    struct HandleHash
    {
        size_t operator()(const Handle<T> &handle) const
        {
            uint64_t key = (uint64_t(handle.generation) << 32) | handle.index;

            return std::hash<uint64_t>{}(key);
        }
    };
} // namespace nitro

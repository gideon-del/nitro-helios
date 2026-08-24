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

        HandleValueType id = INVALID_HANDLE_ID;
        HandleValueType generation = 0;

        bool operator==(const Handle<T> &o) const
        {
            return o.id == id && generation == o.generation;
        }
        bool isValid() const
        {
            return id != INVALID_HANDLE_ID;
        }
    };

    template <typename T>
    struct HandleHash
    {
        size_t operator()(const Handle<T> &handle) const
        {
            uint32_t key = handle.id | handle.generation;

            return std::hash<uint32_t>{}(key);
        }
    };
} // namespace nitro

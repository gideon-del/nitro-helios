#pragma once
#include <cmath>
#include "unordered_map"

namespace nitro::renderer
{
    struct GridCellCoord
    {
        int32_t x, z;

        bool operator==(const GridCellCoord &o) const
        {
            return x == o.x && z == o.z;
        }
    };

    struct GridCellCoordHash
    {
        size_t operator()(const GridCellCoord &cell) const
        {
            uint64_t key = (uint64_t)(uint32_t)cell.x | ((uint64_t)(uint32_t)cell.z << 32);
            return std::hash<uint64_t>{}(key);
        };
    };
} // namespace nitro::renderer

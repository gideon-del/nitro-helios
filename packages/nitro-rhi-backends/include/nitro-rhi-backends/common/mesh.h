#pragma once
#include "vertex.h"
#include <cstdint>

namespace nitro::rhi
{
    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };
} // namespace nitro::rhi

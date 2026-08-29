#pragma once
#include "nitro-core/core.h"
#include <optional>
#include <glm/glm.hpp>
#include "nitro-geometry/geometry.h"
#include "spatial-grid-coord.h"
namespace nitro::renderer
{
    struct MeshTag
    {
    };

    struct MaterialTag
    {
    };

    using MeshHandle = Handle<MeshTag>;
    using MaterialHandle = Handle<MaterialTag>;
    struct MeshInstance
    {
        MeshHandle mesh;
        MaterialHandle material;
        geometry::MeshTransformation transformation;
        std::vector<GridCellCoord> cells;
        glm::vec3 worldAABBMin;
        glm::vec3 worldAABBMax;
        uint8_t dirtyMask = 0;
    };
    using MeshInstanceHandle = Handle<MeshInstance>;

    using MeshInstanceHandleHash = HandleHash<MeshInstance>;
    using OptionalMeshInstanceHandle = std::optional<MeshInstanceHandle>;
} // namespace nitro::renderer

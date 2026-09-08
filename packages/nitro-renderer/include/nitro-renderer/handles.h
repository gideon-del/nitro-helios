#pragma once
#include "nitro-core/core.h"
#include <optional>
#include <glm/glm.hpp>
#include "nitro-geometry/geometry.h"
#include "spatial-grid-coord.h"
#include "nitro-core/uuid.h"
namespace nitro::renderer
{
    struct MeshTag
    {
    };

    struct MaterialTag
    {
    };

    struct Entity;

    using GPUMeshHandle = Handle<MeshTag>;
    using GPUMaterialHandle = Handle<MaterialTag>;
    using GPUMaterialHandleHash = HandleHash<MaterialTag>;
    struct MeshInstance
    {
        GPUMeshHandle mesh;
        GPUMaterialHandle material;
        uint8_t dirtyMask = 0;
        Handle<Entity> entity;
    };
    using MeshInstanceHandle = Handle<MeshInstance>;

    using MeshInstanceHandleHash = HandleHash<MeshInstance>;
    using OptionalMeshInstanceHandle = std::optional<MeshInstanceHandle>;

    using EntityID = UUID;
    struct Entity
    {
        EntityID id;
        std::string name = "Entity";
        geometry::MeshTransformation transformation;
        glm::vec3 worldAABBMin;
        glm::vec3 worldAABBMax;
        std::vector<GridCellCoord> cells;
        OptionalMeshInstanceHandle meshInstance{};
    };

    using EntityHandle = Handle<Entity>;
    using EntityHandleHash = HandleHash<Entity>;

    using OptionalEntityHandle = std::optional<EntityHandle>;
} // namespace nitro::renderer

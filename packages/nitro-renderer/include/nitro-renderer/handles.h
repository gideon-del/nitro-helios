#pragma once
#include "nitro-core/core.h"
#include <optional>
namespace nitro::renderer
{
    struct MeshTag
    {
    };
    struct MeshInstanceTag
    {
    };

    struct MaterialTag
    {
    };

    using MeshHandle = Handle<MeshTag>;
    using MeshInstanceHandle = Handle<MeshInstanceTag>;
    using MaterialHandle = Handle<MaterialTag>;

    using MeshInstanceHandleHash = HandleHash<MeshInstanceTag>;
    using OptionalMeshInstanceHandle = std::optional<MeshInstanceHandle>;
} // namespace nitro::renderer

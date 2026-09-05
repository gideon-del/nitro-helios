#pragma once
#include "nitro-geometry/mesh.h"
#include "registry.h"

namespace nitro::assets
{
    using Mesh = geometry::Mesh;

    using MeshAssetHandle = Handle<AssetEntry<Mesh>>;
    using MeshAssetHandleHash = HandleHash<AssetEntry<Mesh>>;
} // namespace nitro::assets

#pragma once
#include "nitro-core/types/pool.h"
#include <glm/glm.hpp>
#include "texture.h"
namespace nitro::assets
{

    struct MaterialParameters
    {

        glm::vec4 albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        float metallic = 0.0f;
        float roughness = 0.0f;
    };

    struct MaterialTextures
    {
        TextureHandle albedo{},
            normalMap{},
            metallicRoughness{},
            occlusionMap{},
            emissive{};
    };

    struct Material
    {
        MaterialParameters parameters;
        MaterialTextures textures;
    };

    using MaterialAssetHandle = Handle<AssetEntry<Material>>;
    using MaterialAssetHandleHash = HandleHash<AssetEntry<Material>>;

} // namespace nitro::assets

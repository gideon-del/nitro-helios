#pragma once
#include <cstdint>
#include <vector>
#include "registry.h"
#include "nitro-core/types/handle.h"
namespace nitro::assets
{
    struct Texture
    {
        int width, height, channels;
        std::vector<uint8_t> pixels;

        size_t size() const { return size_t(width * height * 4); }
    };

    using TextureHandle = Handle<AssetEntry<Texture>>;
} // namespace nitro::renderer

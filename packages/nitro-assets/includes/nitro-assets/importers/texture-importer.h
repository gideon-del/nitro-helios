#pragma once
#include "nitro-assets/texture.h"
#include <filesystem>
#include "stb_image.h"
namespace nitro::assets
{

    struct TextureImporter
    {
        Texture import(const std::filesystem::path &path)
        {
            int width, height, channels;

            auto raw = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

            if (!raw)
            {
                throw std::runtime_error("Failed to load file at");
            }

            auto size = width * height * 4;

            Texture texture;
            texture.width = width;
            texture.height = height;
            texture.channels = channels;
            texture.pixels.resize(size);

            memcpy(texture.pixels.data(), raw, size);

            stbi_image_free(raw);
            return texture;
        };
    };

} // namespace nitro::assets

#include <nitro-renderer/utils.h>
#include <stb_image.h>
namespace nitro::renderer
{
    rhi::RHITexture *loadHDRImage(std::shared_ptr<rhi::RHIDevice> device, std::string filePath)
    {
        int width, height, channels;
        auto *raw = stbi_loadf(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!raw)
        {
            throw std::runtime_error("File with path " + filePath + " not found");
        }

        rhi::TextureDesc textureDesc;

        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA32;
        textureDesc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead;
        textureDesc.initialData = raw;

        rhi::RHITexture *texture = device->createTexture(textureDesc);

        stbi_image_free(raw);

        return texture;
    }
} // namespace nitro::renderer

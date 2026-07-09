#pragma once
#include <nitro-rhi/rhi-texture.h>
#include <Metal/Metal.hpp>

namespace nitro::rhi::metal
{
    MTL::PixelFormat convertToPixelFormat(const TextureDesc::ImageFormat &format);
    class MetalDevice;
    class MetalTexture : public RHITexture
    {
    public:
        MetalTexture(MetalDevice *device, const TextureDesc &desc);
        ~MetalTexture() override;

        MTL::Texture *texture;
        MTL::SamplerState *samplerState = nullptr;
        uint32_t width;
        uint32_t height;
        MTL::Texture *getFace(int face)
        {
            if (m_isCubeMap)
            {
                return m_faces[face];
            }
            else
            {
                return texture;
            }
        }

    private:
        MetalDevice *m_device;
        std::vector<MTL::Texture *> m_faces{6, nullptr};
        bool m_isCubeMap;
    };
} // namespace nitro::rhi::metal

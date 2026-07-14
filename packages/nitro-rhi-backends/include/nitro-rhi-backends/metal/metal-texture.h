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
        uint32_t mipLevels;
        uint32_t totalLayers;
        MTL::Texture *getFace(int face, int mip = 0)
        {

            return m_faceMipViews[(mip * totalLayers) + face];
        }

    private:
        MetalDevice *m_device;
        std::vector<MTL::Texture *> m_faceMipViews;
        bool m_isCubeMap;
    };
} // namespace nitro::rhi::metal

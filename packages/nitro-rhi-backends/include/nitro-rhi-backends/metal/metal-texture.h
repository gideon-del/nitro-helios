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

    private:
        MetalDevice *m_device;
    };
} // namespace nitro::rhi::metal

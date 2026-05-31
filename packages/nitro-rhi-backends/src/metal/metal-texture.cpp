#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-texture.h>

namespace nitro::rhi::metal
{
    MTL::PixelFormat convertToPixelFormat(const TextureDesc::ImageFormat &format)
    {
        switch (format)
        {
        case TextureDesc::ImageFormat::ColorRGBA8:
            return MTL::PixelFormatRGBA8Unorm;
        case TextureDesc::ImageFormat::ColorSRGBA16:
            return MTL::PixelFormatRGBA16Unorm;
        case TextureDesc::ImageFormat::Depth32Float:
            return MTL::PixelFormatDepth32Float;
        }
        return MTL::PixelFormatRGBA8Unorm;
    }
    MetalTexture::MetalTexture(MetalDevice *device, const TextureDesc &desc) : m_device(device), width(desc.size.width), height(desc.size.height)
    {

        MTL::TextureDescriptor *textureDesc = MTL::TextureDescriptor::texture2DDescriptor(
            convertToPixelFormat(desc.format),
            NS::UInteger(width),
            NS::UInteger(height),
            false);
        MTL::TextureUsage usage = MTL::TextureUsageUnknown;

        if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::RenderTarget) ||
            hasTextureUsageFlag(desc.usage, TextureDesc::Usage::DepthStencil))
        {
            usage |= MTL::TextureUsageRenderTarget;
        }

        if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::ShaderRead))
        {
            usage |= MTL::TextureUsageShaderRead;
        }

        textureDesc->setUsage(usage);
        if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::DepthStencil))
        {
            textureDesc->setStorageMode(MTL::StorageModePrivate);
        }

        texture = m_device->device->newTexture(textureDesc);

        if (!hasTextureUsageFlag(desc.usage, TextureDesc::Usage::DepthStencil) && desc.initialData != nullptr)
        {

            MTL::Region region = MTL::Region::Make2D(NS::UInteger(0),
                                                     NS::UInteger(0),
                                                     NS::UInteger(width),
                                                     NS::UInteger(height));
            texture->replaceRegion(region, NS::UInteger(0), desc.initialData, NS::UInteger(width * 4));
        }

        if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::ShaderRead))
        {

            MTL::SamplerDescriptor *samplerDesc = MTL::SamplerDescriptor::alloc()->init();
            samplerDesc->setMagFilter(MTL::SamplerMinMagFilterLinear);
            samplerDesc->setRAddressMode(MTL::SamplerAddressModeClampToEdge);
            samplerState = m_device->device->newSamplerState(samplerDesc);
            samplerDesc->release();
        }

        textureDesc->release();
    }

    MetalTexture::~MetalTexture()
    {
        if (samplerState)
            samplerState->release();
        if (texture)
            texture->release();
    }
} // namespace nitro::rhi::metal

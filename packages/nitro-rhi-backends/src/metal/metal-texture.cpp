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
        case TextureDesc::ImageFormat::ColorRG8U:
            return MTL::PixelFormatRG8Unorm;
        case TextureDesc::ImageFormat::ColorRGBA16:
            return MTL::PixelFormatRGBA16Float;
        case TextureDesc::ImageFormat::ColorRG16F:
            return MTL::PixelFormatRG16Float;
        case TextureDesc::ImageFormat::ColorRGBA32:
            return MTL::PixelFormatRGBA32Float;
        case TextureDesc::ImageFormat::ColorR32:
            return MTL::PixelFormatR32Float;
        case TextureDesc::ImageFormat::ColorSRGB8:
            return MTL::PixelFormatRGBA8Unorm_sRGB;
        case TextureDesc::ImageFormat::Depth32Float:
            return MTL::PixelFormatDepth32Float;
        case TextureDesc::ImageFormat::Depth32FloatStencil8:
            return MTL::PixelFormatDepth32Float_Stencil8;
        }
        return MTL::PixelFormatRGBA8Unorm;
    }

    MTL::TextureType convertTextureType(rhi::TextureDesc::Type type)
    {
        switch (type)
        {
        case rhi::TextureDesc::Type::Cube:
            return MTL::TextureTypeCube;

        default:
            return MTL::TextureType2D;
        }
    }

    MTL::TextureDescriptor *makeMTLTextureDescriptor(const TextureDesc &desc)
    {
        MTL::TextureDescriptor *textureDesc = MTL::TextureDescriptor::texture2DDescriptor(
            convertToPixelFormat(desc.format),
            NS::UInteger(desc.size.width),
            NS::UInteger(desc.size.height),
            desc.mipmaps > 0);
        MTL::TextureUsage usage = MTL::TextureUsageUnknown;

        if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::RenderTarget) ||
            hasTextureUsageFlag(desc.usage, TextureDesc::Usage::DepthStencil))
        {
            usage |= MTL::TextureUsageRenderTarget;
        }
        if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::Storage))
        {
            usage |= MTL::TextureUsageShaderWrite;
        }

        if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::ShaderRead))
        {
            usage |= MTL::TextureUsageShaderRead;
        }

        textureDesc->setUsage(usage);
        if (desc.mipmaps > 0)
        {
            textureDesc->setMipmapLevelCount(1 + desc.mipmaps);
        }

        // if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::DepthStencil) &&
        //     hasTextureUsageFlag(desc.usage, TextureDesc::Usage::ShaderRead))
        // {
        //     textureDesc->setStorageMode(MTL::StorageModePrivate);
        // }
        // else if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::DepthStencil))
        // {
        //     textureDesc->setStorageMode(MTL::StorageModeMemoryless);
        // }
        // else if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::ShaderRead))
        // {
        //     textureDesc->setStorageMode(MTL::StorageModeShared);
        // }

        textureDesc->setStorageMode(MTL::StorageModePrivate);

        textureDesc->setTextureType(convertTextureType(desc.type));

        return textureDesc;
    }
    MetalTexture::MetalTexture(MetalDevice *device, const TextureDesc &desc) : m_device(device), width(desc.size.width), height(desc.size.height)
    {

        MTL::TextureDescriptor *textureDesc = makeMTLTextureDescriptor(desc);
        texture = m_device->device->newTexture(textureDesc);
        createTextureFaceAndSampler(desc);
        textureDesc->release();
    }

    MetalTexture::MetalTexture(MetalDevice *device, MTL::Texture *texture, const TextureDesc &desc) : m_device(device),
                                                                                                      texture(texture),
                                                                                                      width(desc.size.width),
                                                                                                      height(desc.size.height)
    {
        createTextureFaceAndSampler(desc);
    }

    void MetalTexture::createTextureFaceAndSampler(const TextureDesc &desc)
    {
        m_isCubeMap = desc.type == rhi::TextureDesc::Type::Cube;
        mipLevels = desc.mipmaps;
        totalLayers = m_isCubeMap ? 6 : 1;

        if (!hasTextureUsageFlag(desc.usage, TextureDesc::Usage::DepthStencil) && desc.initialData != nullptr)
        {

            MTL::Region region = MTL::Region::Make2D(NS::UInteger(0),
                                                     NS::UInteger(0),
                                                     NS::UInteger(width),
                                                     NS::UInteger(height));
            size_t bytePerPixel = getImageFormatSize(desc.format);
            texture->replaceRegion(region, NS::UInteger(0), desc.initialData, NS::UInteger(width * bytePerPixel));
        }
        if (!hasTextureUsageFlag(desc.usage, TextureDesc::Usage::DepthStencil))
        {
            for (int mip = 0; mip <= mipLevels; mip++)
            {
                for (int face = 0; face < totalLayers; face++)
                {
                    MTL::TextureViewDescriptor *textureViewDescriptor = MTL::TextureViewDescriptor::alloc()->init();
                    textureViewDescriptor->setPixelFormat(convertToPixelFormat(desc.format));
                    textureViewDescriptor->setTextureType(MTL::TextureType2D);
                    NS::Range levelRange = NS::Range::Make(mip, 1);
                    NS::Range sliceRange = NS::Range::Make(face, 1);
                    textureViewDescriptor->setLevelRange(levelRange);
                    textureViewDescriptor->setSliceRange(sliceRange);
                    m_faceMipViews.push_back(texture->newTextureView(textureViewDescriptor));
                    textureViewDescriptor->release();
                }
            }
        }
        if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::ShaderRead))
        {

            MTL::SamplerDescriptor *samplerDesc = MTL::SamplerDescriptor::alloc()->init();
            samplerDesc->setMagFilter(MTL::SamplerMinMagFilterLinear);
            samplerDesc->setRAddressMode(MTL::SamplerAddressModeRepeat);
            samplerDesc->setSAddressMode(MTL::SamplerAddressModeRepeat);
            samplerDesc->setTAddressMode(MTL::SamplerAddressModeRepeat);
            samplerDesc->setBorderColor(MTL::SamplerBorderColorOpaqueWhite);
            samplerDesc->setMinFilter(MTL::SamplerMinMagFilterLinear);
            if (desc.sampler == TextureDesc::Sampler::Depth && hasTextureUsageFlag(desc.usage, TextureDesc::Usage::DepthStencil))
            {
                samplerDesc->setCompareFunction(MTL::CompareFunctionLessEqual);
            }

            samplerState = m_device->device->newSamplerState(samplerDesc);
            samplerDesc->release();
        }
    }
    MetalTexture::~MetalTexture()
    {

        for (auto &face : m_faceMipViews)
        {
            if (face)
            {
                face->release();
            }
        }
        if (samplerState)
            samplerState->release();
        if (texture)
            texture->release();
    }
} // namespace nitro::rhi::metal

#pragma once
#include <cstdint>

namespace nitro::rhi
{

    enum class ImageLayout
    {
        Undefined,
        ColorAttachment,
        DepthStencilAttachment,
        DepthAttachmentStencilReadOnly,
        DepthReadOnlyStencilAttachment,
        DepthStencilReadOnly,
        ShaderReadOnly,
        TransferSrc,
        TransferDst,
        Present,
        General
    };
    struct TextureDesc
    {
        enum class Sampler
        {
            Depth,
            Sampler2D
        } sampler = Sampler::Sampler2D;
        enum class ImageFormat
        {
            ColorRGBA8,
            ColorRGBA16,
            ColorRGBA32,
            ColorSRGB8,
            Depth32Float,
            Depth32FloatStencil8
        } format;

        enum class Usage : uint32_t
        {
            None = 0,
            RenderTarget = 1 << 0,
            ShaderRead = 1 << 1,
            DepthStencil = 1 << 2,
            Storage = 1 << 3,
        } usage = Usage::None;

        struct Size
        {
            uint32_t width, height;
        } size;
        enum class Type
        {
            Flat,
            Cube
        } type = Type::Flat;
        const void *initialData = nullptr;
        uint32_t mipmaps = 0;
        bool isAliased = false;
    };

    inline bool hasTextureUsageFlag(TextureDesc::Usage value,
                                    TextureDesc::Usage flag)
    {
        return (static_cast<uint32_t>(value) &
                static_cast<uint32_t>(flag)) != 0;
    }

    class RHITexture
    {
    public:
        virtual ~RHITexture() = default;
    };

    inline TextureDesc::Usage operator|(TextureDesc::Usage a, TextureDesc::Usage b)
    {
        return static_cast<TextureDesc::Usage>(
            static_cast<uint32_t>(a) |
            static_cast<uint32_t>(b));
    };

    enum class ResourceState
    {
        Undefined,

        CopySrc,
        CopyDst,

        ShaderRead,
        ShaderWrite,

        RenderTarget,
        DepthWrite,
        DepthRead,

        Present,
    };
    struct TextureSubresource
    {
        uint32_t baseMip = 0;
        uint32_t mipCount = 1;

        uint32_t baseLayer = 0;
        uint32_t layerCount = 1;
    };

    struct TextureBarrier
    {
        RHITexture *texture;
        ResourceState before;
        ResourceState after;

        TextureSubresource subresource;
    };

    inline int getImageFormatSize(TextureDesc::ImageFormat format)
    {
        switch (format)
        {
        case TextureDesc::ImageFormat::ColorRGBA16:
            return 8;
        case TextureDesc::ImageFormat::ColorRGBA32:
            return 16;
        case TextureDesc::ImageFormat::Depth32Float:
            return 4;
        case TextureDesc::ImageFormat::Depth32FloatStencil8:
            return 8;
        default:
            return 4;
        }
    }

}
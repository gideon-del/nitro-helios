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

}
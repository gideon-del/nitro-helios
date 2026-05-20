#pragma once
#include <cstdint>

namespace nitro::rhi
{
    struct TextureDesc
    {
        enum class ImageFormat
        {
            ColorRGBA8,
            ColorSRGBA16,
            Depth32Float,
        } format;

        enum class Usage : uint32_t
        {
            RenderTarget = 1 << 0,
            ShaderRead = 1 << 1
        } usage;

        struct Size
        {
            uint32_t width, height;
        } size;

        const void *initialData = nullptr;
    };

    class RHITexture
    {
    public:
        virtual ~RHITexture() = default;
    };

}
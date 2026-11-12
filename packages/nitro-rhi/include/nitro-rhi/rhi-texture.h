#pragma once
#include <cstdint>
#include <algorithm>
#include <vector>
#include <cassert>
#include "rhi-utils.h"
#include "nitro-core/types/handle.h"
#include "nitro-core/types/size.h"
namespace nitro::rhi
{

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
            ColorRG8U,
            ColorRGBA16,
            ColorRG16F,
            ColorRGBA32,
            ColorR32,
            ColorSRGB8,
            ColorBGRA8_SRGB,
            Depth32Float,
            Depth32FloatStencil8
        } format;

        enum class Usage : uint32_t
        {
            None = 0,
            RenderTarget = 1 << 0,
            ShaderRead = 1 << 1,
            DepthStencil = 1 << 2,
            Storage = 1 << 3
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
        uint32_t mipmaps = 1;
        bool isAliased = false;
    };

    inline bool hasTextureUsageFlag(TextureDesc::Usage value,
                                    TextureDesc::Usage flag)
    {
        return (static_cast<uint32_t>(value) &
                static_cast<uint32_t>(flag)) != 0;
    }

    struct RHITextureTag
    {
    };

    using RHITextureHandle = Handle<RHITextureTag>;

    using RHITextureHandleHash = HandleHash<RHITextureTag>;

    struct TextureState
    {

        TextureState(uint32_t mipCount = 1, uint32_t layers = 1) : perSub(mipCount * layers, ResourceState::Undefined), m_mipCount(mipCount), m_layers(layers)
        {
            assert(mipCount >= 1 && layers >= 1);
        }

        uint32_t mipCount() const { return m_mipCount; }
        uint32_t layerCount() const { return m_layers; }
        ResourceState get(uint32_t sub) const { return uniform ? all : perSub[sub]; }
        uint32_t subIndex(uint32_t mip, uint32_t layer) const { return layer * m_mipCount + mip; }

        void updateRange(ResourceState state, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount)
        {
            uint32_t maxMip = baseMip + mipCount;
            uint32_t maxLayer = baseLayer + layerCount;

            assert(maxMip <= m_mipCount);
            assert(maxLayer <= m_layers);

            for (uint32_t mip = baseMip; mip < maxMip; mip++)
            {
                for (uint32_t layer = baseLayer; layer < maxLayer; layer++)
                {
                    uint32_t sub = subIndex(mip, layer);

                    if (perSub[sub] == state)
                        continue;
                    perSub[sub] = state;
                }
            }

            checkUniform();
        }
        void updateAll(ResourceState state)
        {

            all = state;
            uniform = true;
            std::fill(perSub.begin(), perSub.end(), state);
        }
        const std::vector<ResourceState> &subState() const { return perSub; }
        bool isUniform() const { return uniform; }
        bool isRangeIn(ResourceState s, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount) const
        {
            if (uniform)
                return all == s;

            uint32_t maxMip = baseMip + mipCount;
            uint32_t maxLayer = baseLayer + layerCount;

            assert(maxMip <= m_mipCount);
            assert(maxLayer <= m_layers);
            for (uint32_t mip = baseMip; mip < maxMip; mip++)
            {
                for (uint32_t layer = baseLayer; layer < maxLayer; layer++)
                {
                    uint32_t sub = subIndex(mip, layer);

                    if (perSub[sub] != s)
                        return false;
                }
            }
            return true;
        }

    private:
        void checkUniform()
        {
            for (int i = 1; i < (int)perSub.size(); i++)
            {

                if (perSub[i - 1] != perSub[i])
                {
                    uniform = false;
                    return;
                }
            }
            uniform = true;
            all = perSub[0];
        }

    private:
        uint32_t m_mipCount = 1, m_layers = 1;
        ResourceState all = ResourceState::Undefined;

        std::vector<ResourceState> perSub;
        bool uniform = true;
    };
    class RHITexture
    {
        friend class RHIDevice;
        friend class RHICommandBuffer;
        RHITextureHandle m_handle;

    protected:
        TextureState m_resourceState;

    public:
        virtual ~RHITexture() = default;

        RHITextureHandle getHandle() const { return m_handle; }
        const TextureState &state() const { return m_resourceState; }
        Extent3D extent() const { return m_extent; }
        TextureDesc::ImageFormat format() const { return m_format; }
        uint32_t mipExtent(uint32_t mip, uint32_t size) const { return std::max(1u, size >> mip); }

    protected:
        Extent3D m_extent;
        TextureDesc::ImageFormat m_format;
    };

    inline TextureDesc::Usage operator|(TextureDesc::Usage a, TextureDesc::Usage b)
    {
        return static_cast<TextureDesc::Usage>(
            static_cast<uint32_t>(a) |
            static_cast<uint32_t>(b));
    };
    inline TextureDesc::Usage &operator|=(TextureDesc::Usage &a,
                                          TextureDesc::Usage b)
    {
        a = a | b;
        return a;
    }

    inline bool operator==(const TextureDesc &a, const TextureDesc &b)
    {
        return a.sampler == b.sampler &&
               a.format == b.format &&
               a.usage == b.usage &&
               a.size.width == b.size.width &&
               a.size.height == b.size.height &&
               a.type == b.type &&
               a.initialData == b.initialData &&
               a.mipmaps == b.mipmaps &&
               a.isAliased == b.isAliased;
    }

    struct TextureSubresource
    {
        static constexpr uint32_t Remaining = ~0u;
        uint32_t baseMip = 0;
        uint32_t mipCount = Remaining;

        uint32_t baseLayer = 0;
        uint32_t layerCount = Remaining;

        static TextureSubresource all() { return {}; }

        bool isAll() const
        {
            return baseMip == 0 && mipCount == Remaining && baseLayer == 0 && layerCount == Remaining;
        }

        TextureSubresource resolve(uint32_t texMips, uint32_t texLayers) const
        {
            assert(baseMip < texMips && baseLayer < texLayers);

            TextureSubresource r = *this;

            if (r.mipCount == Remaining)
                r.mipCount = texMips - baseMip;
            if (r.layerCount == Remaining)
                r.layerCount = texLayers - baseLayer;

            assert(r.baseMip + r.mipCount <= texMips);
            assert(r.baseLayer + r.layerCount <= texLayers);
            return r;
        }
    };

    struct TextureBarrier
    {
        RHITexture *texture;
        ResourceState to;
        TextureSubresource range;
    };

    inline int getImageFormatSize(TextureDesc::ImageFormat format)
    {
        switch (format)
        {
        case TextureDesc::ImageFormat::ColorRGBA16:
            return 8;
        case TextureDesc::ImageFormat::ColorRG16F:
            return 4;
        case TextureDesc::ImageFormat::ColorRG8U:
            return 2;
        case TextureDesc::ImageFormat::ColorRGBA32:
            return 16;
        case TextureDesc::ImageFormat::ColorR32:
            return 4;
        case TextureDesc::ImageFormat::Depth32Float:
            return 4;
        case TextureDesc::ImageFormat::Depth32FloatStencil8:
            return 8;
        default:
            return 4;
        }
    }

    inline bool isDepthFormat(TextureDesc::ImageFormat format)
    {
        return format == rhi::TextureDesc::ImageFormat::Depth32Float || format == rhi::TextureDesc::ImageFormat::Depth32FloatStencil8;
    }

}
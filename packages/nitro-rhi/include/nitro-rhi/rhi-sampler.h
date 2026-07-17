#pragma once
#include <cstdint>
#include <nitro-rhi/rhi-pipeline.h>
namespace nitro::rhi
{
    enum class SamplerFilter
    {
        Nearest,
        Linear
    };

    enum class SamplerMipmapMode
    {
        Nearest,
        Linear
    };

    enum class SamplerAddressMode
    {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder
    };

    enum class SamplerBorderColor
    {
        TransparentBlack,
        OpaqueBlack,
        OpaqueWhite
    };

    struct RHISamplerDesc
    {
        SamplerFilter minFilter = SamplerFilter::Linear;
        SamplerFilter magFilter = SamplerFilter::Linear;

        SamplerMipmapMode mipmapMode = SamplerMipmapMode::Linear;

        SamplerAddressMode addressU = SamplerAddressMode::Repeat;
        SamplerAddressMode addressV = SamplerAddressMode::Repeat;
        SamplerAddressMode addressW = SamplerAddressMode::Repeat;

        float mipLodBias = 0.0f;

        bool anisotropy = false;
        float maxAnisotropy = 1.0f;

        bool compareEnabled = false;
        CompareOp compareOp = CompareOp::Always;

        float minLod = 0.0f;
        float maxLod = 1000.0f;

        SamplerBorderColor borderColor =
            SamplerBorderColor::OpaqueBlack;

        bool operator==(const RHISamplerDesc &o) const
        {
            return minFilter == o.minFilter &&
                   magFilter == o.magFilter &&
                   mipmapMode == o.mipmapMode &&
                   addressU == o.addressU &&
                   addressV == o.addressV &&
                   addressW == o.addressW &&
                   mipLodBias == o.mipLodBias &&
                   anisotropy == o.anisotropy &&
                   maxAnisotropy == o.maxAnisotropy &&
                   compareEnabled == o.compareEnabled &&
                   compareOp == o.compareOp &&
                   minLod == o.minLod &&
                   maxLod == o.maxLod &&
                   borderColor == o.borderColor;
        };

        static RHISamplerDesc LinearRepeat()
        {
            RHISamplerDesc desc;

            desc.minFilter = SamplerFilter::Linear;
            desc.magFilter = SamplerFilter::Linear;

            desc.addressU = SamplerAddressMode::Repeat;
            desc.addressV = SamplerAddressMode::Repeat;
            desc.addressW = SamplerAddressMode::Repeat;

            return desc;
        }
        static RHISamplerDesc AnisotropicRepeat()
        {
            RHISamplerDesc desc;

            desc.minFilter = SamplerFilter::Linear;
            desc.magFilter = SamplerFilter::Linear;

            desc.addressU = SamplerAddressMode::Repeat;
            desc.addressV = SamplerAddressMode::Repeat;
            desc.addressW = SamplerAddressMode::Repeat;

            desc.anisotropy = true;
            desc.maxAnisotropy = 16.0f;

            return desc;
        }
        static RHISamplerDesc LinearClamp()
        {
            RHISamplerDesc desc;

            desc.minFilter = SamplerFilter::Linear;
            desc.magFilter = SamplerFilter::Linear;

            desc.addressU = SamplerAddressMode::ClampToEdge;
            desc.addressV = SamplerAddressMode::ClampToEdge;
            desc.addressW = SamplerAddressMode::ClampToEdge;

            return desc;
        }
        static RHISamplerDesc NearestRepeat()
        {
            RHISamplerDesc desc;

            desc.minFilter = SamplerFilter::Nearest;
            desc.magFilter = SamplerFilter::Nearest;

            desc.addressU = SamplerAddressMode::Repeat;
            desc.addressV = SamplerAddressMode::Repeat;
            desc.addressW = SamplerAddressMode::Repeat;

            return desc;
        }
        static RHISamplerDesc NearestClamp()
        {
            RHISamplerDesc desc;

            desc.minFilter = SamplerFilter::Nearest;
            desc.magFilter = SamplerFilter::Nearest;

            desc.addressU = SamplerAddressMode::ClampToEdge;
            desc.addressV = SamplerAddressMode::ClampToEdge;
            desc.addressW = SamplerAddressMode::ClampToEdge;

            return desc;
        }
        static RHISamplerDesc Shadow()
        {
            RHISamplerDesc desc;

            desc.minFilter = SamplerFilter::Linear;
            desc.magFilter = SamplerFilter::Linear;

            desc.addressU = SamplerAddressMode::ClampToBorder;

            desc.addressV = SamplerAddressMode::ClampToBorder;

            desc.addressW = SamplerAddressMode::ClampToBorder;

            desc.compareEnabled = true;
            desc.compareOp = CompareOp::LessOrEqual;

            desc.borderColor = SamplerBorderColor::OpaqueWhite;
            return desc;
        }
    };

    template <typename T>
    inline void hashCombine(size_t &seed, const T &value)
    {

        seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    struct RHISamplerHash
    {
        size_t operator()(const RHISamplerDesc &desc) const
        {
            size_t seed = 0;

            hashCombine<int>(seed, static_cast<int>(desc.addressU));
            hashCombine<int>(seed, static_cast<int>(desc.addressV));
            hashCombine<int>(seed, static_cast<int>(desc.addressW));

            hashCombine<bool>(seed, desc.anisotropy);
            hashCombine<int>(seed, static_cast<int>(desc.borderColor));
            hashCombine<bool>(seed, desc.compareEnabled);
            hashCombine<int>(seed, static_cast<int>(desc.compareOp));
            hashCombine<int>(seed, static_cast<int>(desc.magFilter));
            hashCombine<int>(seed, static_cast<int>(desc.minFilter));
            hashCombine<float>(seed, desc.maxAnisotropy);
            hashCombine<float>(seed, desc.maxLod);
            hashCombine<float>(seed, desc.minLod);
            hashCombine<float>(seed, desc.mipLodBias);
            hashCombine<int>(seed, static_cast<int>(desc.mipmapMode));

            return seed;
        }
    };
} // namespace nitro::rhi

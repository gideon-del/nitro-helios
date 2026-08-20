#include <nitro-rhi-backends/vulkan/vulkan-sampler.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-pipeline.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>

namespace nitro::rhi::vulkan
{

    VkSamplerAddressMode convertToSamplerAddressMode(SamplerAddressMode mode)
    {
        switch (mode)
        {
        case SamplerAddressMode::ClampToBorder:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case SamplerAddressMode::ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case SamplerAddressMode::MirroredRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        default:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }
    VkSamplerMipmapMode convertMipMapMode(SamplerMipmapMode mode)
    {
        switch (mode)
        {
        case SamplerMipmapMode::Nearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
    };

    VkBorderColor convertBorderColor(SamplerBorderColor color)
    {
        switch (color)
        {
        case SamplerBorderColor::TransparentBlack:
            return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        case SamplerBorderColor::OpaqueBlack:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

        default:
            return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        }
    }

    VkFilter convertToFilter(SamplerFilter filter)
    {
        switch (filter)
        {
        case SamplerFilter::Nearest:
            return VK_FILTER_NEAREST;
        default:
            return VK_FILTER_LINEAR;
        }
    }
    VulkanSampler::VulkanSampler(const VulkanDevice &device, const RHISamplerDesc &desc)
    {
        VkSamplerCreateInfo samplerInfo{};

        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.addressModeU = convertToSamplerAddressMode(desc.addressU);
        samplerInfo.addressModeV = convertToSamplerAddressMode(desc.addressV);
        samplerInfo.addressModeW = convertToSamplerAddressMode(desc.addressW);
        samplerInfo.mipmapMode = convertMipMapMode(desc.mipmapMode);
        samplerInfo.minLod = desc.minLod;
        samplerInfo.maxLod = desc.maxLod;
        samplerInfo.mipLodBias = desc.mipLodBias;

        samplerInfo.anisotropyEnable = desc.anisotropy ? VK_TRUE : VK_FALSE;
        samplerInfo.borderColor = convertBorderColor(desc.borderColor);
        samplerInfo.compareEnable = desc.compareEnabled && !device.supportsSamplerComparison() ? VK_TRUE : VK_FALSE;
        samplerInfo.compareOp = convertToCompareOp(desc.compareOp);
        samplerInfo.maxAnisotropy = desc.maxAnisotropy;

        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.magFilter = convertToFilter(desc.magFilter);
        samplerInfo.minFilter = convertToFilter(desc.minFilter);

        checkVkResult(vkCreateSampler(device.device,
                                      &samplerInfo,
                                      nullptr,
                                      &sampler),
                      "Unable to create sampler");
    };
} // namespace nitro::rhi::vulkan

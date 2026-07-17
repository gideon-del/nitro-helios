#pragma once
#include <nitro-rhi/rhi-sampler.h>
#include <vulkan/vulkan.h>

namespace nitro::rhi::vulkan
{

    class VulkanDevice;
    class VulkanSampler
    {
    public:
        VulkanSampler(const VulkanDevice &device, const RHISamplerDesc &desc);
        VkSampler sampler = VK_NULL_HANDLE;
    };

} // namespace nitro::rhi::vulkan

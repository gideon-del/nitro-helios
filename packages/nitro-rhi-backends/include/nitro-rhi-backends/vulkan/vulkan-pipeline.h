#pragma once
#include <nitro-rhi/rhi-pipeline.h>
#include <vulkan/vulkan.h>

namespace nitro::rhi::vulkan
{
    class VulkanDevice;
    class VulkanPipeline : public RHIPipeline
    {
    public:
        VulkanPipeline(VulkanDevice *device, const PipelineDesc &desc);
        ~VulkanPipeline() override;

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineCache cache = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;

    private:
        VulkanDevice *m_device;
    };
} // namespace nitro::rhi::vulkan

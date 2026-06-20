#pragma once
#include <nitro-rhi/rhi-compute-pipeline.h>
#include <vulkan/vulkan.h>
#include "vulkan-pipeline.h"
namespace nitro::rhi::vulkan
{
    class VulkanDevice;
    class VulkanComputePipeline : public RHIComputePipeline
    {
    public:
        VulkanComputePipeline(VulkanDevice *device, const ComputePipelineDesc &desc);
        ~VulkanComputePipeline() override;
        VkPipeline pipeline;
        VkPipelineLayout layout;

    private:
        VulkanDevice *m_device;
    };
} // namespace nitro::rhi::vulkan

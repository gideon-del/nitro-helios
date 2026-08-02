#pragma once
#include <nitro-rhi/rhi-heap.h>
#include <vulkan/vulkan.h>
namespace nitro::rhi::vulkan
{
    class VulkanHeap : public RHIHeap
    {
    public:
        VkDeviceMemory memory;
    };
} // namespace nitro::rhi::vulkan

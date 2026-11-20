#pragma once
#include <vulkan/vulkan.h>
#include "vulkan-descriptor-set.h"
#include "nitro-rhi/rhi-constants.h"
#include "nitro-rhi-backends/vulkan/vulkan-descriptor-layout.h"
namespace nitro::rhi::vulkan
{

    struct FrameDescriptorPools
    {
        std::vector<VkDescriptorPool> used;
        VkDescriptorPool current = VK_NULL_HANDLE;
        std::vector<std::unique_ptr<VulkanDescriptorSet>> wrappers;
        uint32_t wrapperCount = 0;
    };

    class VulkanDevice;

    // vulkan backend only
    class VulkanFrameDescriptorAllocator
    {
    public:
        VulkanFrameDescriptorAllocator(VulkanDevice &device);
        ~VulkanFrameDescriptorAllocator();
        RHIDescriptorSet *allocate(VulkanDescriptorLayout *layout, uint32_t frameIdx);
        void reset(uint32_t frameIdx);

    private:
        VkDescriptorPool acquirePool(FrameDescriptorPools &frame);
        VkDescriptorPool createPool();

    private:
        VulkanDevice &m_device;
        std::array<FrameDescriptorPools, kMaxFramesInFlight> m_frames;
        std::vector<VkDescriptorPool> m_freePools;
    };

} // namespace nitro::rhi::vulkan

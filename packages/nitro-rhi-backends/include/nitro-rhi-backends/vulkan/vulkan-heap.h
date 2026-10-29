#pragma once
#include <nitro-rhi/rhi-heap.h>
#include <vulkan/vulkan.h>
namespace nitro::rhi::vulkan
{
    class VulkanDevice;
    class VulkanHeap : public RHIHeap
    {
    public:
        explicit VulkanHeap(VulkanDevice *device) : m_device(device) {};
        ~VulkanHeap();

        VkDeviceMemory memory = VK_NULL_HANDLE;

    private:
        VulkanDevice *m_device;
    };
} // namespace nitro::rhi::vulkan

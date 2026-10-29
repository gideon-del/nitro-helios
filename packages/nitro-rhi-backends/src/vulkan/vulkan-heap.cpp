#include "nitro-rhi-backends/vulkan/vulkan-heap.h"
#include "nitro-rhi-backends/vulkan/vulkan-device.h"

namespace nitro::rhi::vulkan
{
    VulkanHeap::~VulkanHeap()
    {
        if (memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_device->device, memory, nullptr);
        }
    }
} // namespace nitro::rhi::vulkan

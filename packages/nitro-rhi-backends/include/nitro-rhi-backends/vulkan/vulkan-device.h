#pragma once
#include <nitro-rhi/rhi-device.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace nitro::rhi::vulkan
{
    class VulkanDevice : public RHIDevice
    {

    public:
        void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
        void destroyBuffer(RHIBuffer *buffer) override;

        VkDevice device;
        VmaAllocator allocator;
        VkCommandPool commandPool;
        VkQueue graphicQueue;
        VkQueue presentQueue;
    };
}
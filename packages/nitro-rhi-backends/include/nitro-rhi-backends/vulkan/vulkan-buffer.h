#pragma once
#include <nitro-rhi/rhi-buffer.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
namespace nitro::rhi::vulkan
{
    class VulkanDevice;
    class VulkanBuffer : public RHIBuffer
    {
    public:
        VulkanBuffer(VulkanDevice *device, const BufferDesc &desc);
        ~VulkanBuffer() override;

        void upload(const void *data, size_t size) override;
        size_t getSize() const override;

        VmaAllocation allocation;
        VkBuffer buffer;

    private:
        VulkanDevice *m_device;
        size_t m_size;
    };
}
#pragma once
#include <nitro-rhi/rhi-texture.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace nitro::rhi::vulkan
{
    class VulkanDevice;
    VkFormat convertToFormat(TextureDesc::ImageFormat format);
    class VulkanTexture : public RHITexture
    {

    public:
        VulkanTexture(VulkanDevice *device, const TextureDesc &desc);
        VulkanTexture(VulkanDevice *device, VkImage image, uint32_t width, uint32_t height, VkFormat format);
        ~VulkanTexture() override;

        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkFormat format;
        uint32_t width;
        uint32_t height;

    private:
        VulkanDevice *m_device;
    };

}
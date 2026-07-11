#pragma once
#include <nitro-rhi/rhi-texture.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vector>

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
        VkImageView getFace(int face)
        {
            if (m_isCubeMap)
            {
                return m_faceViews[face];
            }
            else
            {
                return imageView;
            }
        }
        bool isCubeMap() { return m_isCubeMap; }
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageAspectFlags imageAspect;
        VkFormat format;
        uint32_t width;
        uint32_t height;
        uint32_t mipmapLevels = 0;

    private:
        VulkanDevice *m_device;
        std::vector<VkImageView> m_faceViews{6, VK_NULL_HANDLE};
        bool m_isCubeMap;
    };

}
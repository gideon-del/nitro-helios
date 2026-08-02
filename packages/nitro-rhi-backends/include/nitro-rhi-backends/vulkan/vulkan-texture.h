#pragma once
#include <nitro-rhi/rhi-texture.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace nitro::rhi::vulkan
{
    class VulkanDevice;
    VkFormat convertToFormat(TextureDesc::ImageFormat format);
    VkImageType convertVkImageType(rhi::TextureDesc::Type type);
    VkImageUsageFlags convertToImageUsage(TextureDesc::Usage usage);
    VkImageCreateInfo makeVkImageInfo(const TextureDesc &desc);
    class VulkanTexture : public RHITexture
    {

    public:
        VulkanTexture(VulkanDevice *device, const TextureDesc &desc);
        VulkanTexture(VulkanDevice *device, VkImage image, uint32_t width, uint32_t height, VkFormat format);
        VulkanTexture(VulkanDevice *device, VkImage image, const TextureDesc &desc);

        ~VulkanTexture() override;
        VkImageView getFace(int face, int mip = 0)
        {
            return m_faceMipViews[(mip * totalLayers) + face];
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
        uint32_t totalLayers = 1;

    private:
        VulkanDevice *m_device;
        std::vector<VkImageView> m_faceMipViews;
        bool m_isCubeMap;
        bool m_isAliased = false;
        void createImageViews(const TextureDesc &desc);
    };

}
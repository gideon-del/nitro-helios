#include <nitro-rhi-backends/vulkan/vulkan-texture.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>
#include <nitro-rhi-backends/vulkan/vulkan-buffer.h>
namespace nitro::rhi::vulkan
{
    VkFormat convertToFormat(TextureDesc::ImageFormat format)
    {
        switch (format)
        {
        case TextureDesc::ImageFormat::ColorRGBA8:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureDesc::ImageFormat::ColorSRGBA16:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureDesc::ImageFormat::Depth32Float:
            return VK_FORMAT_D32_SFLOAT;
        }
    };
    VkImageAspectFlags convertToAspectFlag(TextureDesc::Usage usage)
    {
        VkImageAspectFlags flags = 0;

        if (hasTextureUsageFlag(usage, TextureDesc::Usage::DepthStencil))
        {
            flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        if (hasTextureUsageFlag(usage, TextureDesc::Usage::RenderTarget))
        {
            flags |= VK_IMAGE_ASPECT_COLOR_BIT;
        }

        if (hasTextureUsageFlag(usage, TextureDesc::Usage::ShaderRead) &&
            !hasTextureUsageFlag(usage, TextureDesc::Usage::DepthStencil))
        {
            flags |= VK_IMAGE_ASPECT_COLOR_BIT;
        }
        return flags;
    }
    VkImageUsageFlags convertToImageUsage(TextureDesc::Usage usage)
    {
        VkImageUsageFlags flags = 0;

        if (hasTextureUsageFlag(usage, TextureDesc::Usage::DepthStencil))
        {
            flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (hasTextureUsageFlag(usage, TextureDesc::Usage::RenderTarget))
        {
            flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (hasTextureUsageFlag(usage, TextureDesc::Usage::ShaderRead))
        {
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        return flags;
    }

    VulkanTexture::VulkanTexture(VulkanDevice *device, const TextureDesc &desc) : m_device(device)
    {
        format = convertToFormat(desc.format);
        width = desc.size.width;
        height = desc.size.height;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.arrayLayers = 1;
        imageInfo.extent = {desc.size.width, desc.size.height, 1};
        imageInfo.format = format;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.mipLevels = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = convertToImageUsage(desc.usage);

        VmaAllocationCreateInfo allocationInfo{};
        allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;

        checkVkResult(vmaCreateImage(
                          m_device->allocator,
                          &imageInfo,
                          &allocationInfo,
                          &image,
                          &allocation,
                          nullptr),
                      "Unable to create Image");

        if (desc.initialData != nullptr && hasTextureUsageFlag(desc.usage, TextureDesc::Usage::ShaderRead))
        {
            BufferDesc stagingDesc;
            stagingDesc.initialData = nullptr;
            stagingDesc.size = width * height * 4;
            stagingDesc.storage = BufferDesc::StorageMode::Shared;
            stagingDesc.usage = BufferDesc::Usage::Staging;
            VulkanBuffer stagingBuffer(m_device, stagingDesc);
            stagingBuffer.upload(desc.initialData, stagingDesc.size);
            m_device->copyBufferToImage(stagingBuffer.buffer, image, stagingDesc.size,
                                        {width, height, 1});
            currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::DepthStencil))
        {
            VkCommandBuffer cmd = m_device->beginOneTimeCommands();

            m_device->transitionImageLayout(
                cmd,
                image,
                0,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT);
            m_device->endOneTimeCommands(cmd);
            currentLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.format = format;
        imageViewInfo.image = image;
        imageViewInfo.subresourceRange.aspectMask = convertToAspectFlag(desc.usage);
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.layerCount = 1;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        checkVkResult(vkCreateImageView(
                          m_device->device,
                          &imageViewInfo,
                          nullptr,
                          &imageView),
                      "Unable to create a image view");
        if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::ShaderRead))
        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_EQUAL;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;

            checkVkResult(vkCreateSampler(m_device->device,
                                          &samplerInfo,
                                          nullptr,
                                          &sampler),
                          "Unable to create sampler");
        }
    }
    VulkanTexture::VulkanTexture(VulkanDevice *device, VkImage existingImage, uint32_t width, uint32_t height, VkFormat surfaceFormat) : m_device(device), width(width), height(height), image(existingImage), format(surfaceFormat)
    {
        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.format = format;
        imageViewInfo.image = image;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.layerCount = 1;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        checkVkResult(vkCreateImageView(
                          m_device->device,
                          &imageViewInfo,
                          nullptr,
                          &imageView),
                      "Unable to create a image view");
    }
    VulkanTexture::~VulkanTexture()
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_device->device, imageView, nullptr);
        }

        if (sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_device->device, sampler, nullptr);
        }

        if (image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_device->allocator, image, allocation);
        }
    }
} // namespace nitro::rhi::vulkan

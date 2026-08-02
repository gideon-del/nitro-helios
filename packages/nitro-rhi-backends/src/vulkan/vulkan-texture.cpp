#include <nitro-rhi-backends/vulkan/vulkan-texture.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>
#include <nitro-rhi-backends/vulkan/vulkan-buffer.h>
namespace nitro::rhi::vulkan
{
    VkImageLayout convertResourceStateToImageLayout(ResourceState state)
    {
        switch (state)
        {
        case ResourceState::Undefined:
            return VK_IMAGE_LAYOUT_UNDEFINED;
            break;
        case ResourceState::CopyDst:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            break;
        case ResourceState::CopySrc:
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            break;
        case ResourceState::DepthRead:
            return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
            break;
        case ResourceState::DepthWrite:
            return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            break;
        case ResourceState::Present:
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            break;
        case ResourceState::RenderTarget:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            break;
        case ResourceState::ShaderRead:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            break;
        case ResourceState::ShaderWrite:
            return VK_IMAGE_LAYOUT_GENERAL;
            break;
        case ResourceState::DepthReadStencilWrite:
            return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
            break;

        default:
            return VK_IMAGE_LAYOUT_UNDEFINED;
            break;
        }
    }

    VkFormat convertToFormat(TextureDesc::ImageFormat format)
    {
        switch (format)
        {
        case TextureDesc::ImageFormat::ColorRGBA8:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureDesc::ImageFormat::ColorSRGB8:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureDesc::ImageFormat::ColorRGBA16:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureDesc::ImageFormat::ColorRGBA32:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case TextureDesc::ImageFormat::Depth32Float:
            return VK_FORMAT_D32_SFLOAT;
        case TextureDesc::ImageFormat::Depth32FloatStencil8:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
        }

        return VK_FORMAT_R8G8B8A8_SRGB;
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
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (hasTextureUsageFlag(usage, TextureDesc::Usage::Storage))
        {
            flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        return flags;
    }

    VkImageType convertVkImageType(rhi::TextureDesc::Type type)
    {
        switch (type)
        {
        case rhi::TextureDesc::Type::Cube:
            return VK_IMAGE_TYPE_2D;
            break;
        case rhi::TextureDesc::Type::Flat:
            return VK_IMAGE_TYPE_2D;
            break;

        default:
            return VK_IMAGE_TYPE_2D;
            break;
        }
    };

    VkImageViewType convertVkImageViewType(rhi::TextureDesc::Type type)
    {
        switch (type)
        {
        case rhi::TextureDesc::Type::Cube:
            return VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case rhi::TextureDesc::Type::Flat:
            return VK_IMAGE_VIEW_TYPE_2D;
            break;

        default:
            return VK_IMAGE_VIEW_TYPE_2D;
            break;
        }
    };
    VkImageCreateInfo makeVkImageInfo(const TextureDesc &desc)
    {
        int totalLayers = (desc.type == TextureDesc::Type::Cube) ? 6 : 1;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.arrayLayers = totalLayers;
        imageInfo.extent = {desc.size.width, desc.size.height, 1};
        imageInfo.format = convertToFormat(desc.format);
        imageInfo.imageType = convertVkImageType(desc.type);
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.mipLevels = 1 + desc.mipmaps;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = convertToImageUsage(desc.usage);

        if (desc.isAliased)
        {
            imageInfo.flags |= VK_IMAGE_CREATE_ALIAS_BIT;
        }

        return imageInfo;
    }

    VulkanTexture::VulkanTexture(VulkanDevice *device, const TextureDesc &desc) : m_device(device),
                                                                                  m_isAliased(desc.isAliased)
    {

        VkImageCreateInfo imageInfo = makeVkImageInfo(desc);

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
        createImageViews(desc);
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

    VulkanTexture::VulkanTexture(VulkanDevice *device,
                                 VkImage image,
                                 const TextureDesc &desc)
        : m_device(device),
          image(image)
    {
        createImageViews(desc);
    }

    void VulkanTexture::createImageViews(const TextureDesc &desc)
    {
        format = convertToFormat(desc.format);
        width = desc.size.width;
        height = desc.size.height;
        mipmapLevels = desc.mipmaps;
        totalLayers = (desc.type == TextureDesc::Type::Cube) ? 6 : 1;

        m_isCubeMap = desc.type == rhi::TextureDesc::Type::Cube;

        imageAspect = convertToAspectFlag(desc.usage);
        if (desc.format == TextureDesc::ImageFormat::Depth32FloatStencil8)
        {
            imageAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        if (desc.initialData != nullptr && hasTextureUsageFlag(desc.usage, TextureDesc::Usage::ShaderRead))
        {
            size_t bytePerPixel = getImageFormatSize(desc.format);
            BufferDesc stagingDesc;
            stagingDesc.initialData = nullptr;
            stagingDesc.size = width * height * bytePerPixel;
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
                imageAspect);
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
        imageViewInfo.subresourceRange.layerCount = totalLayers;

        imageViewInfo.subresourceRange.levelCount = 1 + desc.mipmaps;
        imageViewInfo.viewType = convertVkImageViewType(desc.type);

        checkVkResult(vkCreateImageView(
                          m_device->device,
                          &imageViewInfo,
                          nullptr,
                          &imageView),
                      "Unable to create a image view");

        for (int mip = 0; mip <= mipmapLevels; mip++)
        {
            for (int face = 0; face < totalLayers; face++)
            {
                VkImageViewCreateInfo faceViewInfo{};
                faceViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                faceViewInfo.image = image;
                faceViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                faceViewInfo.format = format;
                faceViewInfo.subresourceRange.aspectMask = convertToAspectFlag(desc.usage);
                faceViewInfo.subresourceRange.baseMipLevel = mip;
                faceViewInfo.subresourceRange.levelCount = 1;
                faceViewInfo.subresourceRange.baseArrayLayer = face;
                faceViewInfo.subresourceRange.layerCount = 1;
                VkImageView faceImageView;
                checkVkResult(vkCreateImageView(m_device->device, &faceViewInfo, nullptr, &faceImageView), "Failed to create cube face");

                m_faceMipViews.push_back(faceImageView);
            }
        }
    }
    VulkanTexture::~VulkanTexture()
    {
        for (auto &faceView : m_faceMipViews)
        {
            if (faceView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_device->device, faceView, nullptr);
            }
        }
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_device->device, imageView, nullptr);
        }

        if (sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_device->device, sampler, nullptr);
        }
        if (m_isAliased && image != VK_NULL_HANDLE)
        {
            vkDestroyImage(m_device->device, image, nullptr);
        }
        else if (image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_device->allocator, image, allocation);
        }
    }
} // namespace nitro::rhi::vulkan

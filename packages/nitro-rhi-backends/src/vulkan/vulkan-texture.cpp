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
            return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureDesc::ImageFormat::ColorRG8U:
            return VK_FORMAT_R8G8_UNORM;
        case TextureDesc::ImageFormat::ColorSRGB8:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureDesc::ImageFormat::ColorRGBA16:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureDesc::ImageFormat::ColorRG16F:
            return VK_FORMAT_R16G16_SFLOAT;
        case TextureDesc::ImageFormat::ColorRGBA32:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case TextureDesc::ImageFormat::ColorR32:
            return VK_FORMAT_R32_SFLOAT;
        case TextureDesc::ImageFormat::Depth32Float:
            return VK_FORMAT_D32_SFLOAT;
        case TextureDesc::ImageFormat::Depth32FloatStencil8:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case TextureDesc::ImageFormat::ColorBGRA8_SRGB:
            return VK_FORMAT_B8G8R8A8_SRGB;
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

        assert(desc.mipmaps > 0);
        imageInfo.mipLevels = desc.mipmaps;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = convertToImageUsage(desc.usage);

        if (desc.isAliased)
        {
            imageInfo.flags |= VK_IMAGE_CREATE_ALIAS_BIT;
        }
        if (desc.type == TextureDesc::Type::Cube)
        {
            imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        return imageInfo;
    }

    VulkanTexture::VulkanTexture(VulkanDevice *device, const TextureDesc &desc) : m_device(device),
                                                                                  m_isAliased(desc.isAliased),
                                                                                  m_ownerShip(ImageOwnership::Vma)
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
    VulkanTexture::VulkanTexture(VulkanDevice *device, VkImage existingImage, uint32_t width, uint32_t height, TextureDesc::ImageFormat surfaceFormat) : m_device(device), width(width), height(height), image(existingImage), format(convertToFormat(surfaceFormat)), m_ownerShip(ImageOwnership::External)
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

        m_extent = {
            .width = width,
            .height = height,
            .depth = 1};

        m_format = surfaceFormat;
        imageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
        mipmapLevels = 1;
        totalLayers = 1;
        m_resourceState = TextureState(1, 1);
    }

    VulkanTexture::VulkanTexture(VulkanDevice *device,
                                 VkImage image,
                                 const TextureDesc &desc)
        : m_device(device),
          image(image),
          m_ownerShip(ImageOwnership::Owned)
    {
        createImageViews(desc);
    }

    void VulkanTexture::createImageViews(const TextureDesc &desc)
    {
        format = convertToFormat(desc.format);
        width = desc.size.width;
        height = desc.size.height;
        assert(desc.mipmaps > 0);
        mipmapLevels = desc.mipmaps;
        totalLayers = (desc.type == TextureDesc::Type::Cube) ? 6 : 1;

        m_resourceState = TextureState(mipmapLevels, totalLayers);
        m_format = desc.format;
        m_extent = {
            .width = desc.size.width,
            .height = desc.size.height,
            .depth = 1};

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
            auto cmd = m_device->createCommandBuffer();
            rhi::TextureBarrier barrier;
            barrier.texture = this;
            barrier.range = {
                .baseMip = 0,
                .mipCount = 1,
                .baseLayer = 0,
                .layerCount = 1};
            barrier.to = rhi::ResourceState::CopyDst;

            cmd->barrier({{barrier}});

            rhi::BufferTextureCopy copyRegion;
            copyRegion.imageExtent.width = width;
            copyRegion.imageExtent.height = height;
            copyRegion.range = {
                .baseMip = 0,
                .mipCount = 1,
                .baseLayer = 0,
                .layerCount = 1};

            cmd->copyBufferToTexture(stagingBuffer, *this, {{copyRegion}});
            barrier.to = rhi::ResourceState::ShaderRead;
            cmd->barrier({{barrier}});
            m_device->endCommandBuffer(cmd);
        }

        if (hasTextureUsageFlag(desc.usage, TextureDesc::Usage::DepthStencil))
        {
            auto cmd = m_device->createCommandBuffer();

            rhi::TextureBarrier barrier;
            barrier.texture = this;
            barrier.range = TextureSubresource::all();
            barrier.to = rhi::ResourceState::DepthWrite;
            cmd->barrier({{barrier}});
            m_device->endCommandBuffer(cmd);
        }

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.format = format;
        imageViewInfo.image = image;

        imageViewInfo.subresourceRange.aspectMask = convertToAspectFlag(desc.usage);

        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.layerCount = totalLayers;

        imageViewInfo.subresourceRange.levelCount = desc.mipmaps;
        imageViewInfo.viewType = convertVkImageViewType(desc.type);

        checkVkResult(vkCreateImageView(
                          m_device->device,
                          &imageViewInfo,
                          nullptr,
                          &imageView),
                      "Unable to create a image view");

        for (int mip = 0; mip < mipmapLevels; mip++)
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

        switch (m_ownerShip)
        {
        case ImageOwnership::Vma:
            if (image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
            {
                vmaDestroyImage(m_device->allocator, image, allocation);
            }
            break;
        case ImageOwnership::Owned:
            if (image != VK_NULL_HANDLE)
            {
                vkDestroyImage(m_device->device, image, nullptr);
            }
        default:
            break;
        }
    }
} // namespace nitro::rhi::vulkan

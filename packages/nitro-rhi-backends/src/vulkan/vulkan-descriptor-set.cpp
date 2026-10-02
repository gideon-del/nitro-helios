#include <nitro-rhi-backends/vulkan/vulkan-descriptor-layout.h>
#include <nitro-rhi-backends/vulkan/vulkan-descriptor-set.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>
#include <nitro-rhi-backends/vulkan/vulkan-texture.h>
#include <nitro-rhi-backends/vulkan/vulkan-buffer.h>
#include <nitro-rhi-backends/vulkan/vulkan-type-conversions.h>

namespace nitro::rhi::vulkan
{
    VulkanDescriptorSet::VulkanDescriptorSet(
        VulkanDevice *device,
        VulkanDescriptorLayout *layout,
        VkDescriptorSet descriptorSet) : m_device(device),
                                         m_layout(layout),
                                         descriptorSet(descriptorSet)
    {
    }
    void VulkanDescriptorSet::writeBuffer(RHIBuffer *buffer, uint32_t binding)
    {

        PendingDescriptorWrite pendingWrite{DescriptorResourceType::Buffer};

        VulkanBuffer *vulkanBuffer = reinterpret_cast<VulkanBuffer *>(buffer);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = vulkanBuffer->buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = vulkanBuffer->getSize();

        pendingWrite.bufferInfo.buffer = vulkanBuffer->buffer;
        pendingWrite.bufferInfo.offset = 0;
        pendingWrite.bufferInfo.range = vulkanBuffer->getSize();

        m_bufferInfos.push_back(std::move(bufferInfo));

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType = m_layout->getBufferType(binding);
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;

        pendingWrite.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pendingWrite.write.descriptorCount = 1;
        pendingWrite.write.descriptorType = m_layout->getBufferType(binding);
        pendingWrite.write.dstSet = descriptorSet;
        pendingWrite.write.dstBinding = binding;
        pendingWrite.write.dstArrayElement = 0;
        pendingWrite.write.pBufferInfo = &pendingWrite.bufferInfo;

        m_writes.push_back(std::move(descriptorWrite));
        m_pendingWrites.push_back(std::move(pendingWrite));
    }

    void VulkanDescriptorSet::writeTexture(const TextureBinding &textureBinding, uint32_t binding, ImageLayout imageLayout)
    {
        VulkanTexture *vulkanTexture = reinterpret_cast<VulkanTexture *>(textureBinding.texture);
        PendingDescriptorWrite pendingWrite{DescriptorResourceType::Texture};
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = toVkImageLayout(imageLayout);
        imageInfo.imageView = vulkanTexture->imageView;
        imageInfo.sampler = m_device->get(textureBinding.sampler).sampler;

        pendingWrite.imageInfo.imageLayout = toVkImageLayout(imageLayout);
        pendingWrite.imageInfo.imageView = vulkanTexture->imageView;
        pendingWrite.imageInfo.sampler = m_device->get(textureBinding.sampler).sampler;

        m_imageInfos.push_back(std::move(imageInfo));

        VkWriteDescriptorSet descriptorWriteTexture{};
        descriptorWriteTexture.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteTexture.descriptorCount = 1;
        descriptorWriteTexture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWriteTexture.dstSet = descriptorSet;
        descriptorWriteTexture.dstBinding = binding;
        descriptorWriteTexture.dstArrayElement = 0;

        pendingWrite.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pendingWrite.write.descriptorCount = 1;
        pendingWrite.write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pendingWrite.write.dstSet = descriptorSet;
        pendingWrite.write.dstBinding = binding;
        pendingWrite.write.dstArrayElement = 0;
        pendingWrite.write.pImageInfo = &pendingWrite.imageInfo;

        m_writes.push_back(std::move(descriptorWriteTexture));
        m_pendingWrites.push_back(std::move(pendingWrite));
    }
    void VulkanDescriptorSet::writeStorageImage(RHITexture *texture, uint32_t binding, ImageLayout imageLayout, TextureSubresource subresource)
    {
        VulkanTexture *vulkanTexture = reinterpret_cast<VulkanTexture *>(texture);
        PendingDescriptorWrite pendingWrite{DescriptorResourceType::Texture};
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = toVkImageLayout(imageLayout);
        imageInfo.imageView = vulkanTexture->getFace(subresource.baseLayer, subresource.baseMip);
        imageInfo.sampler = vulkanTexture->sampler;

        pendingWrite.imageInfo.imageLayout = toVkImageLayout(imageLayout);
        pendingWrite.imageInfo.imageView = vulkanTexture->getFace(subresource.baseLayer, subresource.baseMip);
        pendingWrite.imageInfo.sampler = vulkanTexture->sampler;

        m_imageInfos.push_back(std::move(imageInfo));
        VkWriteDescriptorSet descriptorWriteTexture{};
        descriptorWriteTexture.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteTexture.descriptorCount = 1;
        descriptorWriteTexture.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWriteTexture.dstSet = descriptorSet;
        descriptorWriteTexture.dstBinding = binding;
        descriptorWriteTexture.dstArrayElement = 0;

        pendingWrite.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pendingWrite.write.descriptorCount = 1;
        pendingWrite.write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        pendingWrite.write.dstSet = descriptorSet;
        pendingWrite.write.dstBinding = binding;
        pendingWrite.write.dstArrayElement = 0;
        pendingWrite.write.pImageInfo = &pendingWrite.imageInfo;

        m_writes.push_back(std::move(descriptorWriteTexture));
        m_pendingWrites.push_back(std::move(pendingWrite));
    }
    void VulkanDescriptorSet::writeTextureMip(const TextureBinding &textureBinding, uint32_t binding, ImageLayout imageLayout, TextureSubresource subresource)
    {
        VulkanTexture *vulkanTexture = reinterpret_cast<VulkanTexture *>(textureBinding.texture);
        PendingDescriptorWrite pendingWrite{DescriptorResourceType::Texture};

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = toVkImageLayout(imageLayout);
        imageInfo.imageView = vulkanTexture->getFace(subresource.baseLayer, subresource.baseMip);
        imageInfo.sampler = m_device->get(textureBinding.sampler).sampler;

        pendingWrite.imageInfo.imageLayout = toVkImageLayout(imageLayout);
        pendingWrite.imageInfo.imageView = vulkanTexture->getFace(subresource.baseLayer, subresource.baseMip);
        pendingWrite.imageInfo.sampler = m_device->get(textureBinding.sampler).sampler;

        m_imageInfos.push_back(std::move(imageInfo));
        VkWriteDescriptorSet descriptorWriteTexture{};
        descriptorWriteTexture.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteTexture.descriptorCount = 1;
        descriptorWriteTexture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWriteTexture.dstSet = descriptorSet;
        descriptorWriteTexture.dstBinding = binding;
        descriptorWriteTexture.dstArrayElement = 0;

        pendingWrite.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pendingWrite.write.descriptorCount = 1;
        pendingWrite.write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pendingWrite.write.dstSet = descriptorSet;
        pendingWrite.write.dstBinding = binding;
        pendingWrite.write.dstArrayElement = 0;
        pendingWrite.write.pImageInfo = &pendingWrite.imageInfo;

        m_writes.push_back(std::move(descriptorWriteTexture));
        m_pendingWrites.push_back(std::move(pendingWrite));
    }

    void VulkanDescriptorSet::commit()
    {
        size_t bufferIdx = 0;
        size_t imageIdx = 0;
        // for (auto &write : m_writes)
        // {
        //     if (write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
        //         write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        //     {
        //         write.pBufferInfo = &m_bufferInfos[bufferIdx++];
        //     }
        //     else if (write.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || write.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
        //     {
        //         write.pImageInfo = &m_imageInfos[imageIdx++];
        //     }
        // }
        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve(m_pendingWrites.size());

        for (auto &pendingWrite : m_pendingWrites)
        {
            auto write = pendingWrite.write;

            if (pendingWrite.type == DescriptorResourceType::Buffer)
            {
                write.pBufferInfo = &pendingWrite.bufferInfo;
            }
            if (pendingWrite.type == DescriptorResourceType::Texture)
            {
                write.pImageInfo = &pendingWrite.imageInfo;
            }
            if (pendingWrite.type == DescriptorResourceType::Bindless)
            {
                write.pImageInfo = pendingWrite.bindlessImageInfos.data();
            }
            writes.push_back(write);
        }

        vkUpdateDescriptorSets(m_device->device,
                               static_cast<uint32_t>(writes.size()),
                               writes.data(),
                               0,
                               nullptr);
        m_writes.clear();
        m_bufferInfos.clear();
        m_imageInfos.clear();
        m_bindlessImageInfos.clear();
        m_pendingWrites.clear();
    }

    void VulkanDescriptorSet::writeBindlessTextures(const std::vector<RHITexture *> &textures, uint32_t binding)
    {
        std::cout << "Bindless Texture Size " << textures.size() << " Frame " << m_device->getCurrentFrameIndex() << std::endl;
        if (textures.empty())
            return;
        PendingDescriptorWrite pendingWrite{DescriptorResourceType::Bindless};

        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.reserve(textures.size());
        pendingWrite.bindlessImageInfos.reserve(textures.size());
        for (auto *texture : textures)
        {
            VulkanTexture *vulkanTexture = reinterpret_cast<VulkanTexture *>(texture);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = toVkImageLayout(ImageLayout::ShaderReadOnly);
            imageInfo.imageView = vulkanTexture->imageView;
            imageInfos.push_back(imageInfo);

            pendingWrite.bindlessImageInfos.push_back(imageInfo);
        }

        m_bindlessImageInfos.push_back(std::move(imageInfos));

        VkWriteDescriptorSet descriptorWrite{};

        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.descriptorCount = static_cast<uint32_t>(m_bindlessImageInfos.back().size());
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.pImageInfo = m_bindlessImageInfos.back().data();

        pendingWrite.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pendingWrite.write.descriptorCount = static_cast<uint32_t>(m_bindlessImageInfos.back().size());
        pendingWrite.write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        pendingWrite.write.dstSet = descriptorSet;
        pendingWrite.write.dstBinding = binding;
        pendingWrite.write.dstArrayElement = 0;
        pendingWrite.write.pImageInfo = pendingWrite.bindlessImageInfos.data();

        m_writes.push_back(std::move(descriptorWrite));
        m_pendingWrites.push_back(std::move(pendingWrite));
    }

    void VulkanDescriptorSet::writeSampler(RHISamplerHandle sampler, uint32_t binding)
    {
        PendingDescriptorWrite pendingWrite{DescriptorResourceType::Texture};
        VkDescriptorImageInfo imageInfo{};

        imageInfo.sampler = m_device->get(sampler).sampler;

        pendingWrite.imageInfo.sampler = m_device->get(sampler).sampler;

        m_imageInfos.push_back(std::move(imageInfo));

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;

        pendingWrite.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pendingWrite.write.descriptorCount = 1;
        pendingWrite.write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        pendingWrite.write.dstSet = descriptorSet;
        pendingWrite.write.dstBinding = binding;
        pendingWrite.write.dstArrayElement = 0;
        pendingWrite.write.pImageInfo = &pendingWrite.imageInfo;

        m_writes.push_back(std::move(descriptorWrite));
        m_pendingWrites.push_back(std::move(pendingWrite));
    }
    VulkanDescriptorSet::~VulkanDescriptorSet()
    {

        if (descriptorSet != VK_NULL_HANDLE)
        {
            checkVkResult(vkFreeDescriptorSets(m_device->device, m_layout->descriptorPool, 1, &descriptorSet), "Failed to de-allocate descriptor set");
        }
    };
} // namespace nitro::rhi::vulkan

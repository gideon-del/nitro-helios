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
        VulkanBuffer *vulkanBuffer = reinterpret_cast<VulkanBuffer *>(buffer);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = vulkanBuffer->buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = vulkanBuffer->getSize();
        m_bufferInfos.push_back(std::move(bufferInfo));
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType = m_layout->getBufferType(binding);
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;

        m_writes.push_back(std::move(descriptorWrite));
    }

    void VulkanDescriptorSet::writeTexture(const TextureBinding &textureBinding, uint32_t binding, ImageLayout imageLayout)
    {
        VulkanTexture *vulkanTexture = reinterpret_cast<VulkanTexture *>(textureBinding.texture);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = toVkImageLayout(imageLayout);
        imageInfo.imageView = vulkanTexture->imageView;
        imageInfo.sampler = m_device->get(textureBinding.sampler).sampler;

        m_imageInfos.push_back(std::move(imageInfo));
        VkWriteDescriptorSet descriptorWriteTexture{};
        descriptorWriteTexture.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteTexture.descriptorCount = 1;
        descriptorWriteTexture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWriteTexture.dstSet = descriptorSet;
        descriptorWriteTexture.dstBinding = binding;
        descriptorWriteTexture.dstArrayElement = 0;

        m_writes.push_back(std::move(descriptorWriteTexture));
    }
    void VulkanDescriptorSet::writeStorageImage(RHITexture *texture, uint32_t binding, ImageLayout imageLayout, TextureSubresource subresource)
    {
        VulkanTexture *vulkanTexture = reinterpret_cast<VulkanTexture *>(texture);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = toVkImageLayout(imageLayout);
        imageInfo.imageView = vulkanTexture->getFace(subresource.baseLayer, subresource.baseMip);
        imageInfo.sampler = vulkanTexture->sampler;

        m_imageInfos.push_back(std::move(imageInfo));
        VkWriteDescriptorSet descriptorWriteTexture{};
        descriptorWriteTexture.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteTexture.descriptorCount = 1;
        descriptorWriteTexture.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWriteTexture.dstSet = descriptorSet;
        descriptorWriteTexture.dstBinding = binding;
        descriptorWriteTexture.dstArrayElement = 0;

        m_writes.push_back(std::move(descriptorWriteTexture));
    }

    void VulkanDescriptorSet::commit()
    {
        size_t bufferIdx = 0;
        size_t imageIdx = 0;
        for (auto &write : m_writes)
        {
            if (write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            {
                write.pBufferInfo = &m_bufferInfos[bufferIdx++];
            }
            else if (write.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || write.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
            {
                write.pImageInfo = &m_imageInfos[imageIdx++];
            }
        }
        vkUpdateDescriptorSets(m_device->device,
                               static_cast<uint32_t>(m_writes.size()),
                               m_writes.data(),
                               0,
                               nullptr);
        m_writes.clear();
        m_bufferInfos.clear();
        m_imageInfos.clear();
        m_bindlessImageInfos.clear();
    }

    void VulkanDescriptorSet::writeBindlessTextures(const std::vector<RHITexture *> &textures, uint32_t binding)
    {
        if (textures.empty())
            return;

        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.reserve(textures.size());

        for (auto *texture : textures)
        {
            VulkanTexture *vulkanTexture = reinterpret_cast<VulkanTexture *>(texture);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = toVkImageLayout(ImageLayout::ShaderReadOnly);
            imageInfo.imageView = vulkanTexture->imageView;
            imageInfos.push_back(imageInfo);
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

        m_writes.push_back(std::move(descriptorWrite));
    }

    void VulkanDescriptorSet::writeSampler(RHISamplerHandle sampler, uint32_t binding)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = m_device->get(sampler).sampler;

        m_imageInfos.push_back(std::move(imageInfo));

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;

        m_writes.push_back(std::move(descriptorWrite));
    }
    VulkanDescriptorSet::~VulkanDescriptorSet()
    {

        if (descriptorSet != VK_NULL_HANDLE)
        {
            checkVkResult(vkFreeDescriptorSets(m_device->device, m_layout->descriptorPool, 1, &descriptorSet), "Failed to de-allocate descriptor set");
        }
    };
} // namespace nitro::rhi::vulkan

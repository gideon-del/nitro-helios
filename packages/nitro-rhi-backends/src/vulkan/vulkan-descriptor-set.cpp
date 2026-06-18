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
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;

        m_writes.push_back(std::move(descriptorWrite));
    }

    void VulkanDescriptorSet::writeTexture(RHITexture *texture, uint32_t binding, ImageLayout imageLayout)
    {
        VulkanTexture *vulkanTexture = reinterpret_cast<VulkanTexture *>(texture);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = toVkImageLayout(imageLayout);
        imageInfo.imageView = vulkanTexture->imageView;
        imageInfo.sampler = vulkanTexture->sampler;

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

    void VulkanDescriptorSet::commit()
    {
        size_t bufferIdx = 0;
        size_t imageIdx = 0;
        for (auto &write : m_writes)
        {
            if (write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            {
                write.pBufferInfo = &m_bufferInfos[bufferIdx++];
            }
            else if (write.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
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
    }

    VulkanDescriptorSet::~VulkanDescriptorSet()
    {

        if (descriptorSet != VK_NULL_HANDLE)
        {
            checkVkResult(vkFreeDescriptorSets(m_device->device, m_layout->descriptorPool, 1, &descriptorSet), "Failed to de-allocate descriptor set");
        }
    };
} // namespace nitro::rhi::vulkan

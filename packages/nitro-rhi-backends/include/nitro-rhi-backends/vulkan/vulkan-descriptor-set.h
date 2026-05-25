#pragma once
#include <nitro-rhi/rhi-descriptor-set.h>
#include <vulkan/vulkan.h>

namespace nitro::rhi::vulkan
{
    class VulkanDescriptorLayout;
    class VulkanDevice;
    class VulkanDescriptorSet : public RHIDescriptorSet
    {
    public:
        VulkanDescriptorSet(VulkanDevice *device, VulkanDescriptorLayout *layout, VkDescriptorSet descriptorSet);
        ~VulkanDescriptorSet() override;
        void writeBuffer(RHIBuffer *buffer, uint32_t binding) override;
        void writeTexture(RHITexture *texture, uint32_t binding) override;
        void commit() override;

        VkDescriptorSet descriptorSet;

    private:
        std::vector<VkWriteDescriptorSet> m_writes;
        std::vector<VkDescriptorBufferInfo> m_bufferInfos;
        std::vector<VkDescriptorImageInfo> m_imageInfos;

        VulkanDevice *m_device;
        VulkanDescriptorLayout *m_layout;
    };
} // namespace nitro::rhi::vulkan

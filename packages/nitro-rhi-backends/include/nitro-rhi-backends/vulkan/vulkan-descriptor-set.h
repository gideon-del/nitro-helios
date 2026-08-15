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
        void writeTexture(const TextureBinding &textureBinding, uint32_t binding, ImageLayout imageLayout) override;
        void writeStorageImage(RHITexture *texture, uint32_t binding, ImageLayout imageLayout, TextureSubresource subresource) override;
        void commit() override;
        void writeBindlessTextures(const std::vector<RHITexture *> &textures, uint32_t binding) override;
        void writeTextureMip(const TextureBinding &textureBinding, uint32_t binding, ImageLayout imageLayout, TextureSubresource subresource = TextureSubresource{}) override;
        void writeSampler(RHISamplerHandle sampler, uint32_t binding) override;
        VkDescriptorSet descriptorSet;

    private:
        std::vector<VkWriteDescriptorSet> m_writes;
        std::vector<VkDescriptorBufferInfo> m_bufferInfos;
        std::vector<VkDescriptorImageInfo> m_imageInfos;
        std::vector<std::vector<VkDescriptorImageInfo>> m_bindlessImageInfos;

        VulkanDevice *m_device;
        VulkanDescriptorLayout *m_layout;
    };
} // namespace nitro::rhi::vulkan

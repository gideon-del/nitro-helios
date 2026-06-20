#pragma once
#include <nitro-rhi/rhi-descriptor-layout.h>
#include <vulkan/vulkan.h>
#include <vector>
namespace nitro::rhi::vulkan
{
    class VulkanDevice;
    class VulkanDescriptorLayout : public RHIDescriptorLayout
    {
    public:
        VulkanDescriptorLayout(VulkanDevice *device, const std::vector<RHIDescriptorBinding> bindings);
        ~VulkanDescriptorLayout() override;
        VkDescriptorSet allocateDescriptorSet();
        VkDescriptorType getBufferType(uint32_t binding);
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    private:
        VulkanDevice *m_device;
        std::vector<RHIDescriptorBinding> m_bindings;
    };
} // namespace nitro::rhi::vulkan

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
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    private:
        VulkanDevice *m_device;
    };
} // namespace nitro::rhi::vulkan

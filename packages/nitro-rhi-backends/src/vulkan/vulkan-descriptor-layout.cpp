#include <nitro-rhi-backends/vulkan/vulkan-descriptor-layout.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>

namespace nitro::rhi::vulkan
{
    VkDescriptorType convertToDescriptorType(RHIDescriptorBinding::Type type)
    {
        switch (type)
        {
        case RHIDescriptorBinding::Type::Sampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case RHIDescriptorBinding::Type::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        default:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
    }
    VkShaderStageFlags convertToShaderStage(RHIDescriptorBinding::ShaderStage stage)
    {
        switch (stage)
        {
        case RHIDescriptorBinding::ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case RHIDescriptorBinding::ShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case RHIDescriptorBinding::ShaderStage::Both:
            return VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        default:
            return VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        }
    }

    VkDescriptorSetLayoutBinding convertToVkBinding(const RHIDescriptorBinding &rhiBinding)
    {
        VkDescriptorSetLayoutBinding vkBinding{};
        vkBinding.binding = rhiBinding.binding;
        vkBinding.descriptorCount = 1;
        vkBinding.descriptorType = convertToDescriptorType(rhiBinding.type);
        vkBinding.stageFlags = convertToShaderStage(rhiBinding.stage);
        return vkBinding;
    }

    VkDescriptorPoolSize convertToPoolSize(const RHIDescriptorBinding &rhiBinding)
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.descriptorCount = 10 * VulkanDevice::MAX_FRAMES_IN_FLIGHT;
        poolSize.type = convertToDescriptorType(rhiBinding.type);

        return poolSize;
    }
    VulkanDescriptorLayout::VulkanDescriptorLayout(
        VulkanDevice *device,
        const std::vector<RHIDescriptorBinding> bindings)
        : m_device(device)
    {
        std::vector<VkDescriptorSetLayoutBinding> descriptorBindings(bindings.size());
        std::vector<VkDescriptorPoolSize> poolSizes(bindings.size());
        for (int i = 0; i < bindings.size(); i++)

        {
            descriptorBindings[i] = convertToVkBinding(bindings[i]);
            poolSizes[i] = convertToPoolSize(bindings[i]);
        }

        VkDescriptorSetLayoutCreateInfo descriptorInfo{};
        descriptorInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorInfo.bindingCount = static_cast<uint32_t>(descriptorBindings.size());
        descriptorInfo.pBindings = descriptorBindings.data();
        checkVkResult(vkCreateDescriptorSetLayout(m_device->device, &descriptorInfo, nullptr, &descriptorSetLayout), "Failed to create descriptor layout");

        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptorPoolInfo.maxSets = 10 * VulkanDevice::MAX_FRAMES_IN_FLIGHT;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();

        checkVkResult(vkCreateDescriptorPool(m_device->device, &descriptorPoolInfo, nullptr, &descriptorPool), "Failed to create descriptor pool");
    };

    VulkanDescriptorLayout::~VulkanDescriptorLayout()
    {
        if (descriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(m_device->device, descriptorPool, nullptr);
        if (descriptorSetLayout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(m_device->device, descriptorSetLayout, nullptr);
    }

    VkDescriptorSet VulkanDescriptorLayout::allocateDescriptorSet()
    {

        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &descriptorSetLayout;

        VkDescriptorSet descriptorSet;
        checkVkResult(vkAllocateDescriptorSets(
                          m_device->device,
                          &allocateInfo,
                          &descriptorSet),
                      "Can't allocate descriptor set");
        return descriptorSet;
    }

} // namespace nitro::rhi::vulkan

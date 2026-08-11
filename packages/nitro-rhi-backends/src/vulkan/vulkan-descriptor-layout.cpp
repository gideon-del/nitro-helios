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
        case RHIDescriptorBinding::Type::StorageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case RHIDescriptorBinding::Type::StorageImage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

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
        case RHIDescriptorBinding::ShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
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
        vkBinding.descriptorCount = rhiBinding.isBindlessArray ? rhiBinding.bindlessCount : 1;
        vkBinding.descriptorType = convertToDescriptorType(rhiBinding.type);
        vkBinding.stageFlags = convertToShaderStage(rhiBinding.stage);
        return vkBinding;
    }

    VkDescriptorPoolSize convertToPoolSize(const RHIDescriptorBinding &rhiBinding)
    {
        VkDescriptorPoolSize poolSize{};
        poolSize.descriptorCount = 10000 * VulkanDevice::MAX_FRAMES_IN_FLIGHT;
        poolSize.type = convertToDescriptorType(rhiBinding.type);

        return poolSize;
    }
    VulkanDescriptorLayout::VulkanDescriptorLayout(
        VulkanDevice *device,
        const std::vector<RHIDescriptorBinding> bindings)
        : m_device(device),
          m_bindings(bindings)
    {
        std::vector<VkDescriptorSetLayoutBinding> descriptorBindings(bindings.size());
        std::vector<VkDescriptorPoolSize> poolSizes(bindings.size());
        std::vector<VkDescriptorBindingFlags> bindingFlags(bindings.size());
        bool hasBindless = false;
        for (int i = 0; i < bindings.size(); i++)

        {
            descriptorBindings[i] = convertToVkBinding(bindings[i]);
            poolSizes[i] = convertToPoolSize(bindings[i]);

            if (bindings[i].isBindlessArray)
            {
                bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
                hasBindless = true;
            }
            else
            {
                bindingFlags[i] = 0;
            }
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
        bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
        bindingFlagsInfo.pBindingFlags = bindingFlags.data();

        VkDescriptorSetLayoutCreateInfo descriptorInfo{};
        descriptorInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorInfo.bindingCount = static_cast<uint32_t>(descriptorBindings.size());
        descriptorInfo.pBindings = descriptorBindings.data();
        descriptorInfo.pNext = &bindingFlagsInfo;

        if (hasBindless)
            descriptorInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        checkVkResult(vkCreateDescriptorSetLayout(m_device->device, &descriptorInfo, nullptr, &descriptorSetLayout), "Failed to create descriptor layout");

        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        if (hasBindless)
            descriptorPoolInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        descriptorPoolInfo.maxSets = 1000 * VulkanDevice::MAX_FRAMES_IN_FLIGHT;
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
        static uint32_t allocations = 0;
        std::cout << "Descriptor Set #" << ++allocations << '\n';
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &descriptorSetLayout;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkResult result = vkAllocateDescriptorSets(
            m_device->device,
            &allocateInfo,
            &descriptorSet);
        checkVkResult(result,
                      "Can't allocate descriptor set");

        if (result != VK_SUCCESS || descriptorSet == VK_NULL_HANDLE)
        {
            std::cerr << "vkAllocateDescriptorSets failed with " << result << std::endl;
            throw std::runtime_error("Failed to allocate descriptor sets");
        }
        return descriptorSet;
    }

    VkDescriptorType VulkanDescriptorLayout::getBufferType(uint32_t binding)
    {
        for (auto &layoutBinding : m_bindings)
        {
            if (layoutBinding.binding == binding && layoutBinding.type != RHIDescriptorBinding::Type::Sampler)
            {
                return convertToDescriptorType(layoutBinding.type);
            }
        }

        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }

} // namespace nitro::rhi::vulkan

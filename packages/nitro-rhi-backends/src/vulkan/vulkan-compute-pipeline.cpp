#include <nitro-rhi-backends/vulkan/vulkan-compute-pipeline.h>
#include <nitro-rhi-backends/vulkan/vulkan-descriptor-layout.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>

namespace nitro::rhi::vulkan
{
    VulkanComputePipeline::VulkanComputePipeline(VulkanDevice *device, const ComputePipelineDesc &desc) : m_device(device)
    {

        VkShaderModule computeShaderModule = loadShaderModule(m_device->device, desc.shader);

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.module = computeShaderModule;
        stageInfo.pName = desc.shader.name.c_str();

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        VkPushConstantRange pushConstantRange{};
        if (desc.hasPushConstant)
        {

            pushConstantRange.offset = 0;
            pushConstantRange.size = desc.pushConstantSize;
            pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layoutInfo.pPushConstantRanges = &pushConstantRange;
            layoutInfo.pushConstantRangeCount = 1;
        }
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

        for (auto layout : desc.layouts)
        {
            VulkanDescriptorLayout *descriptorLayout = reinterpret_cast<VulkanDescriptorLayout *>(layout);
            descriptorSetLayouts.push_back(descriptorLayout->descriptorSetLayout);
        }

        layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        layoutInfo.pSetLayouts = descriptorSetLayouts.data();

        checkVkResult(vkCreatePipelineLayout(m_device->device, &layoutInfo, nullptr, &layout), "Failed to create Compute Pipeline Layout");

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.flags = 0;
        pipelineInfo.stage = stageInfo;
        pipelineInfo.layout = layout;

        checkVkResult(vkCreateComputePipelines(
                          m_device->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
                      "Failed to create Compute Pipeline");

        vkDestroyShaderModule(m_device->device, computeShaderModule, nullptr);
    }
    VulkanComputePipeline::~VulkanComputePipeline()
    {
        if (pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_device->device, pipeline, nullptr);
        }
        if (layout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_device->device, layout, nullptr);
        }
    }
} // namespace nitro::rhi::vulkan

#include <nitro-rhi-backends/vulkan/vulkan-pipeline.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-descriptor-layout.h>
#include <nitro-rhi-backends/vulkan/vulkan-texture.h>
#include <fstream>
#include <iostream>
#include <vector>
namespace nitro::rhi::vulkan
{
    std::vector<char> readFile(const std::string filePath)
    {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("File not found " + filePath);
        }

        std::vector<char> buffer(file.tellg());

        file.seekg(0, std::ios::beg);

        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        file.close();

        return buffer;
    }

    VkShaderModule loadShaderModule(VkDevice &device, const ShaderDesc &desc)
    {
        std::vector<char> shaderSourceCode = readFile(desc.filePath);

        VkShaderModuleCreateInfo shaderInfo{};
        shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderInfo.codeSize = shaderSourceCode.size() * sizeof(char);
        shaderInfo.pCode = reinterpret_cast<const uint32_t *>(shaderSourceCode.data());

        VkShaderModule shaderModule;

        checkVkResult(vkCreateShaderModule(device, &shaderInfo, nullptr, &shaderModule), "Shader Module not created");

        return shaderModule;
    }

    VkFormat convertAttributeFormat(const RHIVertexLayout::Attributes::Format &format)
    {
        switch (format)
        {
        case RHIVertexLayout::Attributes::Format::Float:
            return VK_FORMAT_R32_SFLOAT;
        case RHIVertexLayout::Attributes::Format::Float2:
            return VK_FORMAT_R32G32_SFLOAT;
        case RHIVertexLayout::Attributes::Format::Float3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case RHIVertexLayout::Attributes::Format::Float4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        }
    }
    VkVertexInputBindingDescription convertToVertexBinding(const RHIVertexLayout &vertexLayout)
    {
        VkVertexInputBindingDescription binding{};

        binding.binding = vertexLayout.binding;
        binding.stride = vertexLayout.stride;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    VkPrimitiveTopology convertToPrimitive(PipelineTopology topology)
    {
        switch (topology)
        {
        case PipelineTopology::TriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PipelineTopology::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PipelineTopology::PointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        default:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    };

    VkShaderStageFlagBits convertToShaderStage(ShaderStage stage)
    {
        switch (stage)
        {
        case ShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        }

        return VK_SHADER_STAGE_ALL;
    }
    VulkanPipeline::VulkanPipeline(VulkanDevice *device, const PipelineDesc &desc) : m_device(device)
    {

        std::vector<VkShaderModule> shaderModules;
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        for (const auto &shader : desc.shaders)
        {
            shaderModules.push_back(loadShaderModule(m_device->device, shader));
        }

        for (int i = 0; i < desc.shaders.size(); i++)
        {
            auto &shaderModule = shaderModules[i];
            auto &shaderDesc = desc.shaders[i];
            VkPipelineShaderStageCreateInfo shaderStageInfo{};
            shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageInfo.module = shaderModule;
            shaderStageInfo.pName = shaderDesc.name.c_str();
            shaderStageInfo.stage = convertToShaderStage(shaderDesc.stage);

            shaderStages.push_back(shaderStageInfo);
        }

        const auto attributeSize = desc.vertexLayout.attributes.size();
        std::vector<VkVertexInputAttributeDescription> attributes(attributeSize);

        for (int i = 0; i < attributeSize; i++)
        {
            VkVertexInputAttributeDescription attributeDesc{};
            const RHIVertexLayout::Attributes &attribute = desc.vertexLayout.attributes[i];
            attributeDesc.binding = desc.vertexLayout.binding;
            attributeDesc.format = convertAttributeFormat(attribute.format);
            attributeDesc.location = attribute.location;
            attributeDesc.offset = attribute.offset;

            attributes[i] = attributeDesc;
        }

        VkVertexInputBindingDescription binding{};
        VkPipelineVertexInputStateCreateInfo vertextInputInfo{};

        vertextInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        if (!desc.vertexLayout.attributes.empty())
        {
            binding = convertToVertexBinding(desc.vertexLayout);
            vertextInputInfo.vertexBindingDescriptionCount = 1;

            vertextInputInfo.pVertexBindingDescriptions = &binding;
        }
        vertextInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertextInputInfo.pVertexAttributeDescriptions = desc.vertexLayout.attributes.empty() ? nullptr : attributes.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};

        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = convertToPrimitive(desc.topology);
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.dynamicStateCount = 2;
        dynamicStateInfo.pDynamicStates = dynamicStates;

        VkViewport viewport{};

        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)m_device->surface->getWidth();
        viewport.height = (float)m_device->surface->getHeight();
        viewport.maxDepth = 1.0f;
        viewport.minDepth = 0.0f;

        VkRect2D scissors{};

        scissors.extent = {m_device->surface->getWidth(), m_device->surface->getHeight()};
        scissors.offset = {0, 0};

        VkPipelineViewportStateCreateInfo viewportStateInfo{};

        viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateInfo.scissorCount = 1;
        viewportStateInfo.pScissors = &scissors;
        viewportStateInfo.viewportCount = 1;
        viewportStateInfo.pViewports = &viewport;

        VkPipelineColorBlendAttachmentState colorBlendState{};

        colorBlendState.blendEnable = VK_FALSE;
        colorBlendState.colorWriteMask = VK_COLOR_COMPONENT_A_BIT |
                                         VK_COLOR_COMPONENT_B_BIT |
                                         VK_COLOR_COMPONENT_R_BIT |
                                         VK_COLOR_COMPONENT_G_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
        colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendInfo.attachmentCount = 1;
        colorBlendInfo.pAttachments = &colorBlendState;
        colorBlendInfo.logicOpEnable = VK_FALSE;
        colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;

        VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationInfo.depthClampEnable = VK_FALSE;
        rasterizationInfo.depthBiasClamp = 0.0f;
        rasterizationInfo.depthBiasEnable = VK_TRUE;
        rasterizationInfo.lineWidth = 1.0f;
        rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampleInfo{};
        multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleInfo.sampleShadingEnable = VK_FALSE;
        multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthBoundsTestEnable = VK_FALSE;
        depthInfo.depthWriteEnable = VK_TRUE;

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.offset = 0;
        pushConstantRange.size = desc.pushConstantSize;
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkPipelineLayoutCreateInfo pipelayoutInfo{};
        pipelayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelayoutInfo.pushConstantRangeCount = 1;
        pipelayoutInfo.pPushConstantRanges = &pushConstantRange;
        std::vector<VkDescriptorSetLayout> descriptorLayouts;

        for (auto &layout : desc.layouts)
        {
            VulkanDescriptorLayout *vkLayout =
                reinterpret_cast<VulkanDescriptorLayout *>(layout);
            descriptorLayouts.push_back(vkLayout->descriptorSetLayout);
        }

        pipelayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
        pipelayoutInfo.pSetLayouts = descriptorLayouts.data();
        checkVkResult(vkCreatePipelineLayout(m_device->device, &pipelayoutInfo, nullptr, &layout), "Pipeline layout not created");

        VkPipelineCacheCreateInfo pipelineCacheInfo{};
        pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        checkVkResult(vkCreatePipelineCache(m_device->device, &pipelineCacheInfo, nullptr, &cache), "Pipeline cache not created");

        VkGraphicsPipelineCreateInfo pipelineInfo{};

        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = layout;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pViewportState = &viewportStateInfo;
        pipelineInfo.pColorBlendState = &colorBlendInfo;
        pipelineInfo.pVertexInputState = &vertextInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pRasterizationState = &rasterizationInfo;
        pipelineInfo.pDynamicState = &dynamicStateInfo;
        pipelineInfo.pDepthStencilState = &depthInfo;
        pipelineInfo.pMultisampleState = &multisampleInfo;

        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        std::vector<VkFormat> colorFormats;
        renderingInfo.colorAttachmentCount = 0;
        for (auto &colorFormat : desc.colorAttachments)
        {
            colorFormats.push_back(convertToFormat(colorFormat));
        }

        if (desc.colorAttachments.empty())
        {
            colorFormats.push_back(m_device->getSurfaceFormat());
        }
        if (desc.hasColorAttachment)
        {
            renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
            renderingInfo.pColorAttachmentFormats =
                colorFormats.data();
        }

        if (desc.depthTest)
        {
            renderingInfo.depthAttachmentFormat =
                VK_FORMAT_D32_SFLOAT;
        }

        pipelineInfo.pNext = &renderingInfo;

        checkVkResult(vkCreateGraphicsPipelines(m_device->device, cache, 1, &pipelineInfo, nullptr, &pipeline), "Pipeline not created");

        for (auto &shaderModule : shaderModules)
        {
            vkDestroyShaderModule(m_device->device, shaderModule, nullptr);
        }
    }

    VulkanPipeline::~VulkanPipeline()
    {
        if (pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_device->device, pipeline, nullptr);
        }
        if (cache != VK_NULL_HANDLE)
        {
            vkDestroyPipelineCache(m_device->device, cache, nullptr);
        }
        if (layout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_device->device, layout, nullptr);
        }
    }
}
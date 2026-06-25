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

    VkCompareOp convertToCompareOp(CompareOp test)
    {
        switch (test)
        {
        case CompareOp::Less:
            return VK_COMPARE_OP_LESS;
        case CompareOp::LessOrEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareOp::Greater:
            return VK_COMPARE_OP_GREATER;
        case CompareOp::GreaterOrEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CompareOp::Equal:
            return VK_COMPARE_OP_EQUAL;
        case CompareOp::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case CompareOp::Always:
            return VK_COMPARE_OP_ALWAYS;
        default:
            return VK_COMPARE_OP_NEVER;
        }
    }

    VkBlendOp convertBlendOp(RHIBlendDesc::BlendOp operation)
    {
        switch (operation)
        {
        case RHIBlendDesc::BlendOp::Substract:
            return VK_BLEND_OP_SUBTRACT;

        default:
            return VK_BLEND_OP_ADD;
        }
    }

    VkBlendFactor convertBlendFactor(RHIBlendDesc::BlendFactor factor)
    {
        switch (factor)
        {
        case RHIBlendDesc::BlendFactor::Zero:
            return VK_BLEND_FACTOR_ZERO;
        default:
            return VK_BLEND_FACTOR_ONE;
        }
    }
    VkCullModeFlags convertCullMode(PipelineDesc::CullMode cullMode)
    {
        switch (cullMode)
        {
        case PipelineDesc::CullMode::Back:
            return VK_CULL_MODE_BACK_BIT;
        case PipelineDesc::CullMode::Front:
            return VK_CULL_MODE_FRONT_BIT;

        default:
            return VK_CULL_MODE_NONE;
        }
    }
    VkFrontFace convertFrontFace(PipelineDesc::FrontFace frontFace)
    {
        switch (frontFace)
        {
        case PipelineDesc::FrontFace::ClockWise:
            return VK_FRONT_FACE_CLOCKWISE;

        default:
            return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }
    }

    VkStencilOp convertStencilOp(RHIStencilDesc::StencilOp operation)
    {
        switch (operation)
        {
        case RHIStencilDesc::StencilOp::DECREMENT:
            return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case RHIStencilDesc::StencilOp::INCREMENT:
            return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case RHIStencilDesc::StencilOp::KEEP:
            return VK_STENCIL_OP_KEEP;
        case RHIStencilDesc::StencilOp::REPLACE:
            return VK_STENCIL_OP_REPLACE;
        default:
            return VK_STENCIL_OP_ZERO;
        }
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

        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_STENCIL_REFERENCE};

        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.dynamicStateCount = 3;
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

        std::vector<VkPipelineColorBlendAttachmentState> colorBlendStates;

        if (desc.colorAttachments.empty())
        {
            colorBlendState.blendEnable = VK_FALSE;
            colorBlendState.colorWriteMask = VK_COLOR_COMPONENT_A_BIT |
                                             VK_COLOR_COMPONENT_B_BIT |
                                             VK_COLOR_COMPONENT_R_BIT |
                                             VK_COLOR_COMPONENT_G_BIT;
            colorBlendStates.push_back(colorBlendState);
        }

        for (auto &colorAttachment : desc.colorAttachments)
        {
            colorBlendState.blendEnable = VK_FALSE;
            colorBlendState.colorWriteMask = VK_COLOR_COMPONENT_A_BIT |
                                             VK_COLOR_COMPONENT_B_BIT |
                                             VK_COLOR_COMPONENT_R_BIT |
                                             VK_COLOR_COMPONENT_G_BIT;

            if (colorAttachment.blend.enabled)
            {
                colorBlendState.blendEnable = VK_TRUE;
                colorBlendState.colorBlendOp = convertBlendOp(colorAttachment.blend.operation);
                colorBlendState.srcColorBlendFactor = convertBlendFactor(colorAttachment.blend.srcBlendFactor);
                colorBlendState.dstColorBlendFactor = convertBlendFactor(colorAttachment.blend.dstBlendFactor);
            }
            colorBlendStates.push_back(colorBlendState);
        }
        VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
        colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendInfo.attachmentCount = static_cast<uint32_t>(colorBlendStates.size());
        colorBlendInfo.pAttachments = colorBlendStates.data();
        colorBlendInfo.logicOpEnable = VK_FALSE;
        colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;

        VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationInfo.cullMode = convertCullMode(desc.cullMode);
        rasterizationInfo.frontFace = convertFrontFace(desc.frontFace);
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
        depthInfo.depthCompareOp = convertToCompareOp(desc.depthTest);
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthBoundsTestEnable = VK_FALSE;
        depthInfo.depthWriteEnable = desc.depthWrite ? VK_TRUE : VK_FALSE;

        if (desc.stencil.enabled)
        {
            depthInfo.stencilTestEnable = VK_TRUE;
            VkStencilOpState frontOp;
            frontOp.compareMask = desc.stencil.front.compareMask;
            frontOp.compareOp = convertToCompareOp(desc.stencil.front.compareOp);
            frontOp.depthFailOp = convertStencilOp(desc.stencil.front.depthFailOp);
            frontOp.failOp = convertStencilOp(desc.stencil.front.failOp);
            frontOp.passOp = convertStencilOp(desc.stencil.front.passOp);
            frontOp.reference = desc.stencil.front.reference;
            frontOp.writeMask = desc.stencil.front.writeMask;

            depthInfo.front = frontOp;

            VkStencilOpState backOp;
            backOp.compareMask = desc.stencil.back.compareMask;
            backOp.compareOp = convertToCompareOp(desc.stencil.back.compareOp);
            backOp.depthFailOp = convertStencilOp(desc.stencil.back.depthFailOp);
            backOp.failOp = convertStencilOp(desc.stencil.back.failOp);
            backOp.passOp = convertStencilOp(desc.stencil.back.passOp);
            backOp.reference = desc.stencil.back.reference;
            backOp.writeMask = desc.stencil.back.writeMask;
            depthInfo.back = backOp;
        }

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.offset = 0;
        pushConstantRange.size = desc.pushConstantSize;
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkPipelineLayoutCreateInfo pipelayoutInfo{};
        pipelayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        if (desc.hasPushConstant)
        {
            pipelayoutInfo.pushConstantRangeCount = 1;
            pipelayoutInfo.pPushConstantRanges = &pushConstantRange;
        }
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
        for (auto &colorAttachment : desc.colorAttachments)
        {

            colorFormats.push_back(convertToFormat(colorAttachment.format));
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

        if (desc.hasDepth)
        {
            renderingInfo.depthAttachmentFormat =
                convertToFormat(desc.depthAttachmentFormat);
            renderingInfo.stencilAttachmentFormat = desc.stencil.enabled ? convertToFormat(desc.depthAttachmentFormat) : VK_FORMAT_UNDEFINED;
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
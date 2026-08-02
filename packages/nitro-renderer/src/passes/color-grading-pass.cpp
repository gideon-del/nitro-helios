#include <nitro-renderer/passes/color-grading-pass.h>

namespace nitro::renderer
{
    ColorGradingPass::ColorGradingPass(
        std::shared_ptr<rhi::RHIDevice> device,
        uint32_t width,
        uint32_t height,
        std::string shaderDir,
        bool isMetal)
        : m_device(device),
          m_width(width),
          m_height(height)
    {

        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             2},
            {rhi::RHIDescriptorBinding::Type::StorageImage,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             3},
        };

        m_descriptorLayout = m_device->createDescriptorLayout(binding);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.layouts = {m_descriptorLayout};
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(ColorGradingPushConstant);
        std::string shaderPath = shaderDir + "/color-grading/color-grading";

        if (isMetal)
        {
            computePipelineDesc.shader.name = "comp";
            computePipelineDesc.shader.filePath = shaderPath + ".metallib";
        }
        else
        {
            computePipelineDesc.shader.name = "main";
            computePipelineDesc.shader.filePath = shaderPath + ".comp.spv";
        }

        m_computePipeline = m_device->createComputePipeline(computePipelineDesc);

        m_resources.create(g_MAX_FRAMES_IN_FLIGHT,
                           [&](uint32_t frameIdx)
                           {
                               SingleInputPassResource resource;
                               resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
                               return resource;
                           });
    }

    ColorGradingPass::~ColorGradingPass()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyDescriptorSet(resource.descriptorSet);
        }
        m_device->destroyComputePipeline(m_computePipeline);

        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void ColorGradingPass::resize(uint32_t width, uint32_t height)
    {

        for (auto &resource : m_resources)
        {
            resource.lastInputTexture = nullptr;
        }
        m_width = width;
        m_height = height;
    };

    void ColorGradingPass::execute(rhi::RHICommandBuffer *cmd, ColorGradingPushConstant pc, ColorGradingTextures textures)
    {

        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());
        if (resource.lastInputTexture != textures.sceneTexture)
        {
            resource.lastInputTexture = textures.sceneTexture;
            rhi::TextureBinding textureBinding;
            textureBinding.texture = resource.lastInputTexture;
            textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
            resource.descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
            resource.descriptorSet->writeStorageImage(textures.output, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
            resource.descriptorSet->commit();
        }

        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(resource.descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(ColorGradingPushConstant), 1, true);
        uint32_t groupX = (m_width + 15) / 16;
        uint32_t groupY = (m_height + 15) / 16;
        cmd->dispatch(groupX, groupY, 1);
    }
} // namespace nitro::renderer

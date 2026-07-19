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

        rhi::TextureDesc textureDesc;
        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        textureDesc.size = {m_width, m_height};
        textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead | rhi::TextureDesc::Usage::ShaderRead;

        m_colorGradedTexture = m_device->createTexture(textureDesc);

        m_descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

        rhi::RHICommandBuffer *cmd = m_device->createCommandBuffer();
        rhi::TextureBarrier textureBarrier;
        textureBarrier.before = rhi::ResourceState::Undefined;
        textureBarrier.after = rhi::ResourceState::ShaderRead;
        textureBarrier.texture = m_colorGradedTexture;
        cmd->textureBarrier(textureBarrier);
        m_device->endCommandBuffer(cmd);
    }

    ColorGradingPass::~ColorGradingPass()
    {
        m_device->destroyDescriptorSet(m_descriptorSet);
        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyTexture(m_colorGradedTexture);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void ColorGradingPass::resize(uint32_t width, uint32_t height)
    {
        m_device->destroyTexture(m_colorGradedTexture);
        m_lastHdrTexture = nullptr;
        m_width = width;
        m_height = height;

        rhi::TextureDesc textureDesc;
        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        textureDesc.size = {m_width, m_height};
        textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead | rhi::TextureDesc::Usage::ShaderRead;

        m_colorGradedTexture = m_device->createTexture(textureDesc);

        rhi::RHICommandBuffer *cmd = m_device->createCommandBuffer();
        rhi::TextureBarrier textureBarrier;
        textureBarrier.before = rhi::ResourceState::Undefined;
        textureBarrier.after = rhi::ResourceState::ShaderRead;
        textureBarrier.texture = m_colorGradedTexture;
        cmd->textureBarrier(textureBarrier);
        m_device->endCommandBuffer(cmd);
    };

    rhi::RHITexture *ColorGradingPass::execute(rhi::RHICommandBuffer *cmd, ColorGradingPushConstant pc, rhi::RHITexture *hdrTexture)
    {
        if (m_lastHdrTexture != hdrTexture)
        {
            m_lastHdrTexture = hdrTexture;
            rhi::TextureBinding textureBinding;
            textureBinding.texture = m_lastHdrTexture;
            textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
            m_descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
            m_descriptorSet->writeStorageImage(m_colorGradedTexture, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
            m_descriptorSet->commit();
        }

        rhi::TextureBarrier initialBarrier;
        initialBarrier.texture = m_colorGradedTexture;
        initialBarrier.before = rhi::ResourceState::ShaderRead;
        initialBarrier.after = rhi::ResourceState::ShaderWrite;

        cmd->textureBarrier(initialBarrier);
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(m_descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(ColorGradingPushConstant), 1, true);
        uint32_t groupX = (m_width + 15) / 16;
        uint32_t groupY = (m_height + 15) / 16;
        cmd->dispatch(groupX, groupY, 1);

        initialBarrier.before = rhi::ResourceState::ShaderWrite;
        initialBarrier.after = rhi::ResourceState::ShaderRead;
        cmd->textureBarrier(initialBarrier);

        return m_colorGradedTexture;
    }
} // namespace nitro::renderer

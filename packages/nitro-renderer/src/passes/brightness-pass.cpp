#include <nitro-renderer/passes/brightness-pass.h>

namespace nitro::renderer
{
    BrightnessPass::BrightnessPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal)
        : m_device(device),
          m_width(width),
          m_height(height)
    {
        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Compute, 2},
            {rhi::RHIDescriptorBinding::Type::StorageImage, rhi::RHIDescriptorBinding::ShaderStage::Compute, 3},
        };

        m_descriptorLayout = m_device->createDescriptorLayout(binding);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(BrightnessPassPushConstant);
        computePipelineDesc.layouts = {m_descriptorLayout};
        computePipelineDesc.shader.stage = rhi::ShaderStage::Compute;
        std::string shaderPath = shaderDir + "/brightness-extraction/brightness-extraction";
        if (isMetal)
        {
            computePipelineDesc.shader.filePath = shaderPath + ".metallib";
            computePipelineDesc.shader.name = "comp";
        }
        else
        {
            computePipelineDesc.shader.filePath = shaderPath + ".comp.spv";
            computePipelineDesc.shader.name = "main";
        }

        m_computePipeline = m_device->createComputePipeline(computePipelineDesc);

        m_descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

        rhi::TextureDesc textureDesc;

        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        textureDesc.size = {m_width, m_height};
        textureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;

        m_brightnessTexture = m_device->createTexture(textureDesc);

        rhi::RHICommandBuffer *cmd = m_device->createCommandBuffer();

        rhi::TextureBarrier textureBarrier;
        textureBarrier.texture = m_brightnessTexture;
        textureBarrier.before = rhi::ResourceState::Undefined;
        textureBarrier.after = rhi::ResourceState::ShaderRead;
        cmd->textureBarrier(textureBarrier);
        m_device->endCommandBuffer(cmd);
    }

    BrightnessPass::~BrightnessPass()
    {
        m_device->destroyTexture(m_brightnessTexture);
        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyDescriptorSet(m_descriptorSet);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void BrightnessPass::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;

        m_device->destroyTexture(m_brightnessTexture);

        rhi::TextureDesc textureDesc;

        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        textureDesc.size = {m_width, m_height};
        textureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;

        m_brightnessTexture = m_device->createTexture(textureDesc);

        rhi::RHICommandBuffer *cmd = m_device->createCommandBuffer();

        rhi::TextureBarrier textureBarrier;
        textureBarrier.texture = m_brightnessTexture;
        textureBarrier.before = rhi::ResourceState::Undefined;
        textureBarrier.after = rhi::ResourceState::ShaderRead;
        cmd->textureBarrier(textureBarrier);
        m_device->endCommandBuffer(cmd);
    }

    rhi::RHITexture *BrightnessPass::execute(rhi::RHICommandBuffer *cmd, BrightnessPassPushConstant pc, rhi::RHITexture *hdrScene)
    {

        rhi::TextureBarrier textureBarrier;
        textureBarrier.texture = m_brightnessTexture;
        textureBarrier.before = rhi::ResourceState::ShaderRead;
        textureBarrier.after = rhi::ResourceState::ShaderWrite;
        cmd->textureBarrier(textureBarrier);
        cmd->bindComputePipeline(m_computePipeline);

        m_descriptorSet->writeStorageImage(m_brightnessTexture, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
        m_descriptorSet->writeTexture(hdrScene, 2, rhi::ImageLayout::ShaderReadOnly);
        m_descriptorSet->commit();
        cmd->setPushConstant(&pc, sizeof(BrightnessPassPushConstant), 1, true);
        cmd->bindComputeDescriptorSet(m_descriptorSet, 0);

        uint32_t groupX = (m_width + 15) / 16;
        uint32_t groupY = (m_height + 15) / 16;

        cmd->dispatch(groupX, groupY, 1);

        rhi::TextureBarrier barrier;
        barrier.texture = m_brightnessTexture;
        barrier.after = rhi::ResourceState::ShaderRead;
        barrier.before = rhi::ResourceState::ShaderWrite;

        cmd->textureBarrier(barrier);

        return m_brightnessTexture;
    }
} // namespace nitro::renderer

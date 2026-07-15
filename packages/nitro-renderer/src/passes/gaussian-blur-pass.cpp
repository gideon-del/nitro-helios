#include <nitro-renderer/passes/gaussian-blur-pass.h>

namespace nitro::renderer
{
    GaussianBlurPass::GaussianBlurPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal)
        : m_device(device),
          m_width(width),
          m_height(height)
    {
        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Compute, 2},
            {rhi::RHIDescriptorBinding::Type::StorageImage, rhi::RHIDescriptorBinding::ShaderStage::Compute, 3}};
        m_descriptorLayout = m_device->createDescriptorLayout(binding);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(GaussianBlurPushConstant);
        computePipelineDesc.layouts = {m_descriptorLayout};
        computePipelineDesc.shader.stage = rhi::ShaderStage::Compute;
        std::string shaderPath = shaderDir + "/gaussian-blur/gaussian-blur";
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

        m_horizontalDescriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
        m_verticalDescriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

        rhi::TextureDesc textureDesc;

        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        textureDesc.size = {m_width, m_height};
        textureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;

        m_horizontalTexture = m_device->createTexture(textureDesc);
        m_verticalTexture = m_device->createTexture(textureDesc);

        rhi::RHICommandBuffer *cmd = m_device->createCommandBuffer();

        rhi::TextureBarrier textureBarrier;
        textureBarrier.texture = m_horizontalTexture;
        textureBarrier.before = rhi::ResourceState::Undefined;
        textureBarrier.after = rhi::ResourceState::ShaderRead;
        cmd->textureBarrier(textureBarrier);

        textureBarrier.texture = m_verticalTexture;
        cmd->textureBarrier(textureBarrier);
        m_device->endCommandBuffer(cmd);
    }

    GaussianBlurPass::~GaussianBlurPass()
    {
        m_device->destroyComputePipeline(m_computePipeline);
        m_device->destroyDescriptorSet(m_horizontalDescriptorSet);
        m_device->destroyDescriptorSet(m_verticalDescriptorSet);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
        m_device->destroyTexture(m_horizontalTexture);
        m_device->destroyTexture(m_verticalTexture);
    };

    void GaussianBlurPass::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;

        m_device->destroyTexture(m_horizontalTexture);
        m_device->destroyTexture(m_verticalTexture);

        rhi::TextureDesc textureDesc;

        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        textureDesc.size = {m_width, m_height};
        textureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;

        m_horizontalTexture = m_device->createTexture(textureDesc);
        m_verticalTexture = m_device->createTexture(textureDesc);
    }

    rhi::RHITexture *GaussianBlurPass::execute(rhi::RHICommandBuffer *cmd, GaussianBlurPushConstant pc, rhi::RHITexture *inputTexture)
    {

        rhi::TextureBarrier initialBarrier;
        initialBarrier.texture = m_horizontalTexture;
        initialBarrier.before = rhi::ResourceState::ShaderRead;
        initialBarrier.after = rhi::ResourceState::ShaderWrite;
        cmd->textureBarrier(initialBarrier);
        m_horizontalDescriptorSet->writeTexture(inputTexture, 2, rhi::ImageLayout::ShaderReadOnly);
        m_horizontalDescriptorSet->writeStorageImage(m_horizontalTexture, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
        m_horizontalDescriptorSet->commit();

        pc.horizontal = 1;
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(m_horizontalDescriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(GaussianBlurPushConstant), 1, true);

        uint32_t groupX = (m_width + 15) / 16;
        uint32_t groupY = (m_height + 15) / 16;

        cmd->dispatch(groupX, groupY, 1);

        rhi::TextureBarrier textureBarrier{};
        textureBarrier.texture = m_horizontalTexture;
        textureBarrier.before = rhi::ResourceState::ShaderWrite;
        textureBarrier.after = rhi::ResourceState::ShaderRead;

        cmd->textureBarrier(textureBarrier);

        initialBarrier.texture = m_verticalTexture;
        cmd->textureBarrier(initialBarrier);

        m_verticalDescriptorSet->writeTexture(m_horizontalTexture, 2, rhi::ImageLayout::ShaderReadOnly);
        m_verticalDescriptorSet->writeStorageImage(m_verticalTexture, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
        m_verticalDescriptorSet->commit();

        pc.horizontal = 1;
        cmd->bindComputePipeline(m_computePipeline);
        cmd->bindComputeDescriptorSet(m_verticalDescriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(GaussianBlurPushConstant), 1, true);
        cmd->dispatch(groupX, groupY, 1);
        textureBarrier.texture = m_verticalTexture;
        cmd->textureBarrier(textureBarrier);

        return m_verticalTexture;
    };

} // namespace nitro::renderer

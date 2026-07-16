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
        m_downSampleDescriptorLayout = m_device->createDescriptorLayout(binding);

        std::vector<rhi::RHIDescriptorBinding> upsampleBinding{
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Compute, 2},
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Compute, 3},
            {rhi::RHIDescriptorBinding::Type::StorageImage, rhi::RHIDescriptorBinding::ShaderStage::Compute, 4}};
        m_upSampleDescriptorLayout = m_device->createDescriptorLayout(upsampleBinding);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(GaussianBlurPushConstant);
        computePipelineDesc.layouts = {m_downSampleDescriptorLayout};
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

        m_downSampleComputePipeline = m_device->createComputePipeline(computePipelineDesc);

        computePipelineDesc.pushConstantSize = sizeof(UpSamplePushConstant);
        computePipelineDesc.layouts = {m_upSampleDescriptorLayout};
        computePipelineDesc.shader.stage = rhi::ShaderStage::Compute;
        shaderPath = shaderDir + "/upsample-pass/upsample-pass";
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

        m_upSampleComputePipeline = m_device->createComputePipeline(computePipelineDesc);

        for (uint32_t i = 0; i < GaussianBlurPass::GAUSSIAN_MIP_COUNT; i++)
        {
            rhi::TextureDesc textureDesc;
            uint32_t textureWidth = std::max(m_width >> (i + 1), 1u);
            uint32_t textureHeight = std::max(m_height >> (i + 1), 1u);
            textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
            textureDesc.size = {textureWidth, textureHeight};
            textureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;

            GaussianBlurMip downSampleMip;
            downSampleMip.texture = m_device->createTexture(textureDesc);
            downSampleMip.horizontalScratch = m_device->createTexture(textureDesc);
            downSampleMip.width = textureWidth;
            downSampleMip.height = textureHeight;
            downSampleMip.horizontalDescriptorSet = m_device->createDescriptorSet(m_downSampleDescriptorLayout);
            downSampleMip.verticalDescriptorSet = m_device->createDescriptorSet(m_downSampleDescriptorLayout);
            m_downSampleBlurMips[i] = downSampleMip;

            GaussianBlurMip upSampleMip;
            upSampleMip.texture = m_device->createTexture(textureDesc);
            upSampleMip.horizontalScratch = m_device->createTexture(textureDesc);
            upSampleMip.width = textureWidth;
            upSampleMip.height = textureHeight;
            upSampleMip.horizontalDescriptorSet = m_device->createDescriptorSet(m_upSampleDescriptorLayout);
            upSampleMip.verticalDescriptorSet = m_device->createDescriptorSet(m_upSampleDescriptorLayout);
            m_upSampleBlurMips[i] = upSampleMip;
        }
        rhi::RHICommandBuffer *cmd = m_device->createCommandBuffer();

        for (uint32_t i = 0; i < GaussianBlurPass::GAUSSIAN_MIP_COUNT; i++)
        {
            rhi::TextureBarrier textureBarrier;
            textureBarrier.texture = m_downSampleBlurMips[i].horizontalScratch;
            textureBarrier.before = rhi::ResourceState::Undefined;
            textureBarrier.after = rhi::ResourceState::ShaderRead;
            cmd->textureBarrier(textureBarrier);

            textureBarrier.texture = m_downSampleBlurMips[i].texture;
            cmd->textureBarrier(textureBarrier);
            textureBarrier.texture = m_upSampleBlurMips[i].horizontalScratch;
            cmd->textureBarrier(textureBarrier);
            textureBarrier.texture = m_upSampleBlurMips[i].texture;
            cmd->textureBarrier(textureBarrier);
        }

        m_device->endCommandBuffer(cmd);

        for (int i = 0; i < GAUSSIAN_MIP_COUNT; i++)
        {
            if (i != 0)
            {
                m_downSampleBlurMips[i].horizontalDescriptorSet->writeTexture(m_downSampleBlurMips[i - 1].texture, 2, rhi::ImageLayout::ShaderReadOnly);
                m_downSampleBlurMips[i].horizontalDescriptorSet->writeStorageImage(m_downSampleBlurMips[i].horizontalScratch, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
                m_downSampleBlurMips[i].horizontalDescriptorSet->commit();
            }

            m_downSampleBlurMips[i].verticalDescriptorSet->writeTexture(m_downSampleBlurMips[i].horizontalScratch, 2, rhi::ImageLayout::ShaderReadOnly);
            m_downSampleBlurMips[i].verticalDescriptorSet->writeStorageImage(m_downSampleBlurMips[i].texture, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
            m_downSampleBlurMips[i].verticalDescriptorSet->commit();
        }

        for (int i = GAUSSIAN_MIP_COUNT - 2; i >= 0; i--)
        {
            m_upSampleBlurMips[i].verticalDescriptorSet->writeTexture(i == GAUSSIAN_MIP_COUNT - 2 ? m_downSampleBlurMips[GAUSSIAN_MIP_COUNT - 1].texture : m_upSampleBlurMips[i + 1].texture, 2, rhi::ImageLayout::ShaderReadOnly);
            m_upSampleBlurMips[i].verticalDescriptorSet->writeTexture(m_downSampleBlurMips[i].texture, 3, rhi::ImageLayout::ShaderReadOnly);
            m_upSampleBlurMips[i].verticalDescriptorSet->writeStorageImage(m_upSampleBlurMips[i].texture, 4, rhi::ImageLayout::General, rhi::TextureSubresource{});
            m_upSampleBlurMips[i].verticalDescriptorSet->commit();
        }
    }

    GaussianBlurPass::~GaussianBlurPass()
    {
        m_device->destroyComputePipeline(m_downSampleComputePipeline);
        m_device->destroyComputePipeline(m_upSampleComputePipeline);

        for (uint32_t i = 0; i < GaussianBlurPass::GAUSSIAN_MIP_COUNT; i++)
        {
            m_device->destroyTexture(m_downSampleBlurMips[i].texture);
            m_device->destroyTexture(m_downSampleBlurMips[i].horizontalScratch);
            m_device->destroyTexture(m_upSampleBlurMips[i].texture);
            m_device->destroyTexture(m_upSampleBlurMips[i].horizontalScratch);
            m_device->destroyDescriptorSet(m_downSampleBlurMips[i].horizontalDescriptorSet);
            m_device->destroyDescriptorSet(m_downSampleBlurMips[i].verticalDescriptorSet);
            m_device->destroyDescriptorSet(m_upSampleBlurMips[i].horizontalDescriptorSet);
            m_device->destroyDescriptorSet(m_upSampleBlurMips[i].verticalDescriptorSet);
        }

        m_device->destroyDescriptorLayout(m_upSampleDescriptorLayout);
        m_device->destroyDescriptorLayout(m_downSampleDescriptorLayout);
    };

    void GaussianBlurPass::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;

        for (uint32_t i = 0; i < GaussianBlurPass::GAUSSIAN_MIP_COUNT; i++)
        {
            m_device->destroyTexture(m_downSampleBlurMips[i].texture);
            m_device->destroyTexture(m_downSampleBlurMips[i].horizontalScratch);
            m_device->destroyTexture(m_upSampleBlurMips[i].texture);
            m_device->destroyTexture(m_upSampleBlurMips[i].horizontalScratch);
        }
        for (uint32_t i = 0; i < GaussianBlurPass::GAUSSIAN_MIP_COUNT; i++)
        {
            rhi::TextureDesc textureDesc;
            uint32_t textureWidth = std::max(m_width >> (i + 1), 1u);
            uint32_t textureHeight = std::max(m_height >> (i + 1), 1u);
            textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
            textureDesc.size = {textureWidth, textureHeight};
            textureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;

            m_downSampleBlurMips[i].texture = m_device->createTexture(textureDesc);
            m_downSampleBlurMips[i].horizontalScratch = m_device->createTexture(textureDesc);
            m_downSampleBlurMips[i].width = textureWidth;
            m_downSampleBlurMips[i].height = textureHeight;

            GaussianBlurMip upSampleMip;
            m_upSampleBlurMips[i].texture = m_device->createTexture(textureDesc);
            m_upSampleBlurMips[i].horizontalScratch = m_device->createTexture(textureDesc);
            m_upSampleBlurMips[i].width = textureWidth;
            m_upSampleBlurMips[i].height = textureHeight;
        }
        rhi::RHICommandBuffer *cmd = m_device->createCommandBuffer();

        for (uint32_t i = 0; i < GaussianBlurPass::GAUSSIAN_MIP_COUNT; i++)
        {
            rhi::TextureBarrier textureBarrier;
            textureBarrier.texture = m_downSampleBlurMips[i].horizontalScratch;
            textureBarrier.before = rhi::ResourceState::Undefined;
            textureBarrier.after = rhi::ResourceState::ShaderRead;
            cmd->textureBarrier(textureBarrier);

            textureBarrier.texture = m_downSampleBlurMips[i].texture;
            cmd->textureBarrier(textureBarrier);
            textureBarrier.texture = m_upSampleBlurMips[i].horizontalScratch;
            cmd->textureBarrier(textureBarrier);
            textureBarrier.texture = m_upSampleBlurMips[i].texture;
            cmd->textureBarrier(textureBarrier);
        }
        m_device->endCommandBuffer(cmd);

        for (int i = 0; i < GAUSSIAN_MIP_COUNT; i++)
        {
            if (i != 0)
            {
                m_downSampleBlurMips[i].horizontalDescriptorSet->writeTexture(m_downSampleBlurMips[i - 1].texture, 2, rhi::ImageLayout::ShaderReadOnly);
                m_downSampleBlurMips[i].horizontalDescriptorSet->writeStorageImage(m_downSampleBlurMips[i].horizontalScratch, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
                m_downSampleBlurMips[i].horizontalDescriptorSet->commit();
            }

            m_downSampleBlurMips[i].verticalDescriptorSet->writeTexture(m_downSampleBlurMips[i].horizontalScratch, 2, rhi::ImageLayout::ShaderReadOnly);
            m_downSampleBlurMips[i].verticalDescriptorSet->writeStorageImage(m_downSampleBlurMips[i].texture, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
            m_downSampleBlurMips[i].verticalDescriptorSet->commit();
        }

        for (int i = GAUSSIAN_MIP_COUNT - 2; i >= 0; i--)
        {
            m_upSampleBlurMips[i].verticalDescriptorSet->writeTexture(i == GAUSSIAN_MIP_COUNT - 2 ? m_downSampleBlurMips[GAUSSIAN_MIP_COUNT - 1].texture : m_upSampleBlurMips[i + 1].texture, 2, rhi::ImageLayout::ShaderReadOnly);
            m_upSampleBlurMips[i].verticalDescriptorSet->writeTexture(m_downSampleBlurMips[i].texture, 3, rhi::ImageLayout::ShaderReadOnly);
            m_upSampleBlurMips[i].verticalDescriptorSet->writeStorageImage(m_upSampleBlurMips[i].texture, 4, rhi::ImageLayout::General, rhi::TextureSubresource{});
            m_upSampleBlurMips[i].verticalDescriptorSet->commit();
        }

        m_lastInputTexture = nullptr;
    }

    rhi::RHITexture *GaussianBlurPass::execute(rhi::RHICommandBuffer *cmd, GaussianBlurPushConstant pc, rhi::RHITexture *inputTexture)
    {

        if (m_lastInputTexture != inputTexture)
        {
            m_lastInputTexture = inputTexture;
            m_downSampleBlurMips[0].horizontalDescriptorSet->writeTexture(m_lastInputTexture, 2, rhi::ImageLayout::ShaderReadOnly);
            m_downSampleBlurMips[0].horizontalDescriptorSet->writeStorageImage(m_downSampleBlurMips[0].horizontalScratch, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
            m_downSampleBlurMips[0].horizontalDescriptorSet->commit();
        }
        for (int i = 0; i < GAUSSIAN_MIP_COUNT; i++)
        {
            pc.outputTextureSize = glm::vec2(float(m_downSampleBlurMips[i].width), float(m_downSampleBlurMips[i].height));
            pc.inputTextureSize = i == 0 ? pc.inputTextureSize : glm::vec2(float(m_downSampleBlurMips[i - 1].width), float(m_downSampleBlurMips[i - 1].height));

            rhi::TextureBarrier initialBarrier;
            initialBarrier.texture = m_downSampleBlurMips[i].horizontalScratch;
            initialBarrier.before = rhi::ResourceState::ShaderRead;
            initialBarrier.after = rhi::ResourceState::ShaderWrite;

            cmd->textureBarrier(initialBarrier);

            pc.horizontal = 1;
            cmd->bindComputePipeline(m_downSampleComputePipeline);
            cmd->bindComputeDescriptorSet(m_downSampleBlurMips[i].horizontalDescriptorSet, 0);
            cmd->setPushConstant(&pc, sizeof(GaussianBlurPushConstant), 1, true);

            uint32_t groupX = (m_downSampleBlurMips[i].width + 15) / 16;
            uint32_t groupY = (m_downSampleBlurMips[i].height + 15) / 16;

            cmd->dispatch(groupX, groupY, 1);

            rhi::TextureBarrier textureBarrier{};
            textureBarrier.texture = m_downSampleBlurMips[i].horizontalScratch;
            textureBarrier.before = rhi::ResourceState::ShaderWrite;
            textureBarrier.after = rhi::ResourceState::ShaderRead;

            cmd->textureBarrier(textureBarrier);

            initialBarrier.texture = m_downSampleBlurMips[i].texture;
            cmd->textureBarrier(initialBarrier);

            pc.horizontal = 0;
            cmd->bindComputePipeline(m_downSampleComputePipeline);
            cmd->bindComputeDescriptorSet(m_downSampleBlurMips[i].verticalDescriptorSet, 0);
            cmd->setPushConstant(&pc, sizeof(GaussianBlurPushConstant), 1, true);
            cmd->dispatch(groupX, groupY, 1);
            textureBarrier.texture = m_downSampleBlurMips[i].texture;
            cmd->textureBarrier(textureBarrier);
        }

        for (int i = GAUSSIAN_MIP_COUNT - 2; i >= 0; i--)
        {
            UpSamplePushConstant pc;
            pc.textureSize = glm::vec2(float(m_upSampleBlurMips[i].width), float(m_upSampleBlurMips[i].height));

            rhi::TextureBarrier initialBarrier;
            initialBarrier.texture = m_upSampleBlurMips[i].texture;
            initialBarrier.before = rhi::ResourceState::ShaderRead;
            initialBarrier.after = rhi::ResourceState::ShaderWrite;

            cmd->textureBarrier(initialBarrier);

            cmd->bindComputePipeline(m_upSampleComputePipeline);
            cmd->bindComputeDescriptorSet(m_upSampleBlurMips[i].verticalDescriptorSet, 0);
            cmd->setPushConstant(&pc, sizeof(UpSamplePushConstant), 1, true);

            uint32_t groupX = (m_upSampleBlurMips[i].width + 15) / 16;
            uint32_t groupY = (m_upSampleBlurMips[i].height + 15) / 16;

            cmd->dispatch(groupX, groupY, 1);

            rhi::TextureBarrier textureBarrier{};
            textureBarrier.texture = m_upSampleBlurMips[i].texture;
            textureBarrier.before = rhi::ResourceState::ShaderWrite;
            textureBarrier.after = rhi::ResourceState::ShaderRead;

            cmd->textureBarrier(textureBarrier);
        }

        return m_upSampleBlurMips[0].texture;
    };

} // namespace nitro::renderer

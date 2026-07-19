#include <nitro-renderer/passes/ssao-pass.h>
#include <nitro-renderer/utils.h>

namespace nitro::renderer
{
    SSAOPass::SSAOPass(std::shared_ptr<rhi::RHIDevice> device,
                       uint32_t width,
                       uint32_t height,
                       rhi::RHITexture *gDepth,
                       rhi::RHITexture *gNormal,
                       std::string shaderDir,
                       bool isMetal)
        : m_device(device),
          m_width(width),
          m_height(height)
    {
        m_noiseTexture = generateSSAONoiseTexture(m_device);
        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             2},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             3},
            {rhi::RHIDescriptorBinding::Type::StorageImage,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             4},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             5},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             6}};

        m_ssaoDescriptorLayout = m_device->createDescriptorLayout(binding);

        std::vector<rhi::RHIDescriptorBinding> blurBinding{
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             2},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             3},
            {rhi::RHIDescriptorBinding::Type::StorageImage,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             4},
        };

        m_blurDescriptorLayout = m_device->createDescriptorLayout(blurBinding);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.layouts = {m_ssaoDescriptorLayout};
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(SSAOPushConstant);
        std::string shaderPath = shaderDir + "/ssao/ssao";

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

        m_ssaoComputePipeline = m_device->createComputePipeline(computePipelineDesc);

        computePipelineDesc.layouts = {m_blurDescriptorLayout};
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(SSAOBlurPushConstant);
        shaderPath = shaderDir + "/ssao-blur/ssao-blur";

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
        m_blurComputePipeline = m_device->createComputePipeline(computePipelineDesc);

        rhi::TextureDesc textureDesc;
        textureDesc.size = {m_width, m_height};
        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        textureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;

        m_ssaoTexture = m_device->createTexture(textureDesc);
        m_blurTexture = m_device->createTexture(textureDesc);

        m_resources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [&](uint32_t frameIdx)
            {
                SSAOResources resource;

                rhi::BufferDesc bufferDesc;
                bufferDesc.size = sizeof(glm::vec4) * 200;
                bufferDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                bufferDesc.usage = rhi::BufferDesc::Usage::Storage;

                resource.randomSampleBuffer = m_device->createBuffer(bufferDesc);

                resource.descriptorSet = m_device->createDescriptorSet(m_ssaoDescriptorLayout);

                rhi::TextureBinding textureBinding;
                textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
                textureBinding.texture = gDepth;

                resource.descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
                textureBinding.texture = gNormal;
                resource.descriptorSet->writeTexture(textureBinding, 3, rhi::ImageLayout::ShaderReadOnly);
                textureBinding.texture = m_noiseTexture;
                textureBinding.sampler = m_device->defaultSamplers().nearestRepeat;
                resource.descriptorSet->writeTexture(textureBinding, 6, rhi::ImageLayout::ShaderReadOnly);
                resource.descriptorSet->writeStorageImage(m_ssaoTexture, 4, rhi::ImageLayout::General, rhi::TextureSubresource{});
                resource.descriptorSet->writeBuffer(resource.randomSampleBuffer, 5);
                resource.descriptorSet->commit();

                return resource;
            });

        m_blurDescriptorSet = m_device->createDescriptorSet(m_blurDescriptorLayout);

        rhi::TextureBinding blurTextureBinding;
        blurTextureBinding.texture = gDepth;
        blurTextureBinding.sampler = m_device->defaultSamplers().linearRepeat;
        m_blurDescriptorSet->writeTexture(blurTextureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
        blurTextureBinding.texture = m_ssaoTexture;
        m_blurDescriptorSet->writeTexture(blurTextureBinding, 3, rhi::ImageLayout::ShaderReadOnly);
        m_blurDescriptorSet->writeStorageImage(m_blurTexture, 4, rhi::ImageLayout::General, rhi::TextureSubresource{});
        m_blurDescriptorSet->commit();

        rhi::RHICommandBuffer *cmd = m_device->createCommandBuffer();

        rhi::TextureBarrier textureBarrier;
        textureBarrier.texture = m_ssaoTexture;
        textureBarrier.before = rhi::ResourceState::Undefined;
        textureBarrier.after = rhi::ResourceState::ShaderRead;

        cmd->textureBarrier(textureBarrier);
        textureBarrier.texture = m_blurTexture;
        cmd->textureBarrier(textureBarrier);

        m_device->endCommandBuffer(cmd);
    }

    SSAOPass::~SSAOPass()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyDescriptorSet(resource.descriptorSet);
            m_device->destroyBuffer(resource.randomSampleBuffer);
        }

        m_device->destroyComputePipeline(m_blurComputePipeline);
        m_device->destroyComputePipeline(m_ssaoComputePipeline);
        m_device->destroyDescriptorLayout(m_blurDescriptorLayout);
        m_device->destroyDescriptorLayout(m_ssaoDescriptorLayout);
        m_device->destroyDescriptorSet(m_blurDescriptorSet);

        m_device->destroyTexture(m_ssaoTexture);
        m_device->destroyTexture(m_blurTexture);
        m_device->destroyTexture(m_noiseTexture);
    }

    void SSAOPass::resize(uint32_t width,
                          uint32_t height,
                          rhi::RHITexture *gDepth,
                          rhi::RHITexture *gNormal)
    {
        m_width = width;
        m_height = height;

        m_device->destroyTexture(m_ssaoTexture);
        m_device->destroyTexture(m_blurTexture);

        rhi::TextureDesc textureDesc;
        textureDesc.size = {m_width, m_height};
        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        textureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;

        m_ssaoTexture = m_device->createTexture(textureDesc);
        m_blurTexture = m_device->createTexture(textureDesc);

        rhi::RHICommandBuffer *cmd = m_device->createCommandBuffer();
        rhi::TextureBarrier textureBarrier;
        textureBarrier.texture = m_ssaoTexture;
        textureBarrier.before = rhi::ResourceState::Undefined;
        textureBarrier.after = rhi::ResourceState::ShaderRead;

        cmd->textureBarrier(textureBarrier);

        textureBarrier.texture = m_blurTexture;
        cmd->textureBarrier(textureBarrier);
        m_device->endCommandBuffer(cmd);

        for (auto &resource : m_resources)
        {
            rhi::TextureBinding textureBinding;
            textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
            textureBinding.texture = gDepth;

            resource.descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
            textureBinding.texture = gNormal;
            resource.descriptorSet->writeTexture(textureBinding, 3, rhi::ImageLayout::ShaderReadOnly);
            resource.descriptorSet->writeStorageImage(m_ssaoTexture, 4, rhi::ImageLayout::General, rhi::TextureSubresource{});
            textureBinding.texture = m_noiseTexture;
            textureBinding.sampler = m_device->defaultSamplers().nearestRepeat;
            resource.descriptorSet->writeTexture(textureBinding, 6, rhi::ImageLayout::ShaderReadOnly);
            resource.descriptorSet->writeBuffer(resource.randomSampleBuffer, 5);
            resource.descriptorSet->commit();
        }

        rhi::TextureBinding blurTextureBinding;
        blurTextureBinding.texture = gDepth;
        blurTextureBinding.sampler = m_device->defaultSamplers().linearRepeat;
        m_blurDescriptorSet->writeTexture(blurTextureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
        blurTextureBinding.texture = m_ssaoTexture;
        m_blurDescriptorSet->writeTexture(blurTextureBinding, 3, rhi::ImageLayout::ShaderReadOnly);
        m_blurDescriptorSet->writeStorageImage(m_blurTexture, 4, rhi::ImageLayout::General, rhi::TextureSubresource{});
        m_blurDescriptorSet->commit();
    }

    void SSAOPass::execute(rhi::RHICommandBuffer *cmd, SSAOPushConstant pc, const std::vector<glm::vec4> &samples, float depthSigma)
    {

        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        resource.randomSampleBuffer->upload(samples.data(), sizeof(glm::vec4) * samples.size());
        rhi::TextureBarrier textureBarrier;
        textureBarrier.texture = m_ssaoTexture;
        textureBarrier.before = rhi::ResourceState::ShaderRead;
        textureBarrier.after = rhi::ResourceState::ShaderWrite;

        cmd->textureBarrier(textureBarrier);
        cmd->bindComputePipeline(m_ssaoComputePipeline);
        cmd->bindComputeDescriptorSet(resource.descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(SSAOPushConstant), 1, true);
        uint32_t groupSizeX = (m_width + 15) / 16;
        uint32_t groupSizeY = (m_height + 15) / 16;

        cmd->dispatch(groupSizeX, groupSizeY, 1);

        textureBarrier.before = rhi::ResourceState::ShaderWrite;
        textureBarrier.after = rhi::ResourceState::ShaderRead;

        cmd->textureBarrier(textureBarrier);

        textureBarrier.texture = m_blurTexture;
        textureBarrier.before = rhi::ResourceState::ShaderRead;
        textureBarrier.after = rhi::ResourceState::ShaderWrite;

        cmd->textureBarrier(textureBarrier);
        SSAOBlurPushConstant blurPc;
        blurPc.depthSigma = depthSigma;
        blurPc.textureSize = pc.textureSize;
        cmd->bindComputePipeline(m_blurComputePipeline);
        cmd->bindComputeDescriptorSet(m_blurDescriptorSet, 0);
        cmd->setPushConstant(&blurPc, sizeof(SSAOBlurPushConstant), 1, true);
        cmd->dispatch(groupSizeX, groupSizeY, 1);

        textureBarrier.before = rhi::ResourceState::ShaderWrite;
        textureBarrier.after = rhi::ResourceState::ShaderRead;

        cmd->textureBarrier(textureBarrier);
    };
} // namespace nitro::renderer

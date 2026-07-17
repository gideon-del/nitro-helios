#include <nitro-renderer/passes/auto-exposure-pass.h>

namespace nitro::renderer
{
    AutoExposurePass::AutoExposurePass(std::shared_ptr<rhi::RHIDevice> device,
                                       uint32_t width,
                                       uint32_t height,
                                       std::string shaderDir,
                                       bool isMetal)
        : m_device(device),
          m_width(width),
          m_height(height)
    {
        std::vector<rhi::RHIDescriptorBinding> bindings{
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             2},
            {rhi::RHIDescriptorBinding::Type::StorageImage,
             rhi::RHIDescriptorBinding::ShaderStage::Compute,
             3}};
        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::ComputePipelineDesc computePipelineDesc;
        computePipelineDesc.layouts = {m_descriptorLayout};
        computePipelineDesc.hasPushConstant = true;
        computePipelineDesc.pushConstantSize = sizeof(AutoExposurePushConstant);
        std::string shaderPath = shaderDir + "/luminance/luminance";

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

        m_luminanceComputePipeline = m_device->createComputePipeline(computePipelineDesc);

        shaderPath = shaderDir + "/luminance-downsample/luminance-downsample";

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
        m_downSampleComputePipeline = m_device->createComputePipeline(computePipelineDesc);

        m_readbackBuffers.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [&](uint32_t frameIdx)
            {
                ReadbackBuffer readbackBuffer;
                rhi::BufferDesc bufferDesc;
                bufferDesc.usage = rhi::BufferDesc::Usage::TransferDst;
                bufferDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                bufferDesc.size = 16;

                readbackBuffer.buffer = m_device->createBuffer(bufferDesc);
                return readbackBuffer;
            });
        m_allocateMips();
    }

    void AutoExposurePass::m_allocateMips()
    {
        m_autoExposureMips.clear();

        uint32_t texWidth = std::max(m_width >> 1, 1u);
        uint32_t texHeight = std::max(m_height >> 1, 1u);

        rhi::TextureDesc textureDesc;

        textureDesc.usage = rhi::TextureDesc::Usage::Storage | rhi::TextureDesc::Usage::ShaderRead;
        textureDesc.size = {texWidth, texHeight};
        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA32;

        AutoExposureMips firstMip;
        firstMip.width = texWidth;
        firstMip.height = texHeight;
        firstMip.mip = m_device->createTexture(textureDesc);
        firstMip.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

        m_autoExposureMips.push_back(firstMip);

        while (texWidth > 1 || texHeight > 1)
        {
            int lastMip = m_autoExposureMips.size() - 1;
            texWidth = std::max(texWidth >> 1, 1u);
            texHeight = std::max(texHeight >> 1, 1u);

            textureDesc.size = {texWidth, texHeight};
            AutoExposureMips autoExposureMip;
            autoExposureMip.width = texWidth;
            autoExposureMip.height = texHeight;
            autoExposureMip.mip = m_device->createTexture(textureDesc);
            autoExposureMip.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
            rhi::TextureBinding textureBinding;

            textureBinding.texture = m_autoExposureMips[lastMip].mip;
            textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
            autoExposureMip.descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
            autoExposureMip.descriptorSet->writeStorageImage(autoExposureMip.mip, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
            autoExposureMip.descriptorSet->commit();

            m_autoExposureMips.push_back(autoExposureMip);
        }

        rhi::RHICommandBuffer *cmd = m_device->createCommandBuffer();

        for (auto &exposureMip : m_autoExposureMips)
        {
            rhi::TextureBarrier textureBarrier;
            textureBarrier.before = rhi::ResourceState::Undefined;
            textureBarrier.after = rhi::ResourceState::ShaderRead;
            textureBarrier.texture = exposureMip.mip;

            cmd->textureBarrier(textureBarrier);
        }

        m_device->endCommandBuffer(cmd);
    };

    void AutoExposurePass::m_destroyMips()
    {

        for (auto &exposureMip : m_autoExposureMips)
        {
            m_device->destroyTexture(exposureMip.mip);
            m_device->destroyDescriptorSet(exposureMip.descriptorSet);
        }
    }
    AutoExposurePass::~AutoExposurePass()
    {
        m_destroyMips();

        m_device->destroyComputePipeline(m_luminanceComputePipeline);
        m_device->destroyComputePipeline(m_downSampleComputePipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);

        for (auto &readbackBuffer : m_readbackBuffers)
        {
            m_device->destroyBuffer(readbackBuffer.buffer);
        }
    };

    void AutoExposurePass::resize(uint32_t width, uint32_t height)
    {

        m_destroyMips();
        m_width = width;
        m_height = height;
        m_allocateMips();
        m_lastInputTexture = nullptr;
    };

    float AutoExposurePass::execute(rhi::RHICommandBuffer *cmd, AutoExposurePushConstant pc, rhi::RHITexture *inputTexture, ToneMapSettings &toneMapSettings,
                                    float deltaTime)
    {

        auto &readbackBuffer = m_readbackBuffers.current(m_device->getCurrentFrameIndex());
        pc.outputTextureSize = glm::vec2(float(m_autoExposureMips[0].width), float(m_autoExposureMips[0].height));

        rhi::TextureBarrier initialBarrier;
        initialBarrier.before = rhi::ResourceState::ShaderRead;
        initialBarrier.after = rhi::ResourceState::ShaderWrite;
        initialBarrier.texture = m_autoExposureMips[0].mip;

        cmd->textureBarrier(initialBarrier);

        if (m_lastInputTexture != inputTexture)
        {
            m_lastInputTexture = inputTexture;

            rhi::TextureBinding textureBinding;

            textureBinding.texture = inputTexture;
            textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
            m_autoExposureMips[0].descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
            m_autoExposureMips[0].descriptorSet->writeStorageImage(m_autoExposureMips[0].mip, 3, rhi::ImageLayout::General, rhi::TextureSubresource{});
            m_autoExposureMips[0].descriptorSet->commit();
        }

        uint32_t groupSizeX = (m_autoExposureMips[0].width + 15) / 16;
        uint32_t groupSizeY = (m_autoExposureMips[0].height + 15) / 16;

        cmd->bindComputePipeline(m_luminanceComputePipeline);
        cmd->bindComputeDescriptorSet(m_autoExposureMips[0].descriptorSet, 0);
        cmd->setPushConstant(&pc, sizeof(AutoExposurePushConstant), 1, true);
        cmd->dispatch(groupSizeX, groupSizeY, 1);
        rhi::TextureBarrier finalBarrier;
        finalBarrier.before = rhi::ResourceState::ShaderWrite;
        finalBarrier.after = rhi::ResourceState::ShaderRead;
        finalBarrier.texture = m_autoExposureMips[0].mip;

        cmd->textureBarrier(finalBarrier);

        for (int i = 1; i < m_autoExposureMips.size(); i++)
        {

            pc.outputTextureSize = glm::vec2(float(m_autoExposureMips[i].width), float(m_autoExposureMips[i].height));

            initialBarrier.texture = m_autoExposureMips[i].mip;

            cmd->textureBarrier(initialBarrier);

            uint32_t groupSizeX = (m_autoExposureMips[i].width + 15) / 16;
            uint32_t groupSizeY = (m_autoExposureMips[i].height + 15) / 16;

            cmd->bindComputePipeline(m_downSampleComputePipeline);
            cmd->bindComputeDescriptorSet(m_autoExposureMips[i].descriptorSet, 0);
            cmd->setPushConstant(&pc, sizeof(AutoExposurePushConstant), 1, true);
            cmd->dispatch(groupSizeX, groupSizeY, 1);

            if (i == m_autoExposureMips.size() - 1)
                break;
            finalBarrier.texture = m_autoExposureMips[i].mip;

            cmd->textureBarrier(finalBarrier);
        };
        auto &finalMip = m_autoExposureMips.back();

        rhi::TextureBarrier transferBarrier;
        transferBarrier.texture = finalMip.mip;
        transferBarrier.before = rhi::ResourceState::ShaderWrite;
        transferBarrier.after = rhi::ResourceState::CopySrc;
        cmd->textureBarrier(transferBarrier);

        cmd->copyTextureToBuffer(finalMip.mip, readbackBuffer.buffer);
        float *bufferData = static_cast<float *>(readbackBuffer.buffer->map());

        float avgLum = bufferData[0];
        avgLum = glm::exp(avgLum);

        float targetExposure = 0.18f / std::max(avgLum, 0.001f);
        readbackBuffer.buffer->unmap();

        const float brighteningSpeed = 2.0f;
        const float darkeningSpeed = 1.0f;
        float speed = (targetExposure < toneMapSettings.exposure) ? brighteningSpeed : darkeningSpeed;

        float t = 1.0f - glm::exp(-speed * deltaTime);
        finalBarrier.before = rhi::ResourceState::CopySrc;
        finalBarrier.texture = finalMip.mip;

        cmd->textureBarrier(finalBarrier);

        return glm::mix(toneMapSettings.exposure, targetExposure, t);
    }
} // namespace nitro::renderer

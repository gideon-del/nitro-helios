#include <nitro-renderer/passes/skybox-pass.h>

namespace nitro::renderer
{
    SkyboxPass::SkyboxPass(std::shared_ptr<rhi::RHIDevice> device,
                           uint32_t width,
                           uint32_t height,
                           std::string shaderDir,
                           bool isMetal)
        : m_device(device),
          m_width(width),
          m_height(height)
    {
        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::UniformBuffer, rhi::RHIDescriptorBinding::ShaderStage::Fragment, 2},
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Fragment, 3},
        };
        m_descriptorLayout = m_device->createDescriptorLayout(binding);

        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.depthWrite = false;
        pipelineDesc.hasDepth = false;
        pipelineDesc.colorAttachments = {rhi::TextureDesc::ImageFormat::ColorRGBA16};
        pipelineDesc.layouts = {m_descriptorLayout};
        pipelineDesc.hasPushConstant = false;
        pipelineDesc.cullMode = rhi::PipelineDesc::CullMode::None;
        std::string shaderPath = shaderDir + "/skybox-pass/skybox-pass";

        if (isMetal)
        {
            pipelineDesc.shaders.push_back({"vs", shaderPath + ".metallib", rhi::ShaderStage::Vertex});
            pipelineDesc.shaders.push_back({"fs", shaderPath + ".metallib", rhi::ShaderStage::Fragment});
        }
        else
        {
            pipelineDesc.shaders.push_back({"main", shaderPath + ".vert.spv", rhi::ShaderStage::Vertex});
            pipelineDesc.shaders.push_back({"main", shaderPath + ".frag.spv", rhi::ShaderStage::Fragment});
        }

        m_pipeline = m_device->createPipeline(pipelineDesc);

        rhi::TextureDesc textureDesc;

        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA16;
        textureDesc.size = {m_width, m_height};
        textureDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        m_skyboxTexture = m_device->createTexture(textureDesc);

        m_resources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [&](uint32_t frameIdx)
            {
                SkyboxPassResource resource;

                rhi::BufferDesc uboDesc;
                uboDesc.size = sizeof(SkyboxPassUBO);
                uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

                resource.uniformBuffer = m_device->createBuffer(uboDesc);

                resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

                return resource;
            });
    }

    void SkyboxPass::bindResources(const RGResources &resources, const SkyboxTextures &textures)
    {
        for (auto &resource : m_resources)
        {
            resource.descriptorSet->writeBuffer(resource.uniformBuffer, 2);
            rhi::TextureBinding textureBinding;
            textureBinding.texture = textures.cubemapTexture;
            textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
            resource.descriptorSet->writeTexture(textureBinding, 3, rhi::ImageLayout::ShaderReadOnly);
            resource.descriptorSet->commit();
        }

        if (m_renderPass)
        {
            m_device->destroyRenderPass(m_renderPass);
        }

        rhi::RenderPassDesc renderPassDesc;

        rhi::RenderPassDesc::Attachment colorAttachment;
        colorAttachment.texture = resources.getTexture(textures.skybox);
        colorAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        colorAttachment.store = rhi::RenderPassDesc::StoreOp::Store;

        renderPassDesc.colorAttachments = {colorAttachment};
        renderPassDesc.width = m_width;
        renderPassDesc.height = m_height;
        m_renderPass = m_device->createRenderPass(renderPassDesc);
    }

    SkyboxPass::~SkyboxPass()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyBuffer(resource.uniformBuffer);
            m_device->destroyDescriptorSet(resource.descriptorSet);
        }

        m_device->destroyRenderPass(m_renderPass);
        m_device->destroyPipeline(m_pipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
        m_device->destroyTexture(m_skyboxTexture);
    }

    void SkyboxPass::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    }

    void SkyboxPass::execute(rhi::RHICommandBuffer *cmd, SkyboxPassUBO ubo)
    {
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        cmd->beginRenderPass(m_renderPass);
        cmd->bindPipeline(m_pipeline);
        resource.uniformBuffer->upload(&ubo, sizeof(SkyboxPassUBO));

        cmd->bindDescriptorSet(resource.descriptorSet, 0);

        rhi::RHIViewport viewport;

        viewport.width = m_width;
        viewport.height = m_height;
        cmd->setViewPort(viewport);

        rhi::RHIScissor scissor;
        scissor.width = m_width;
        scissor.height = m_height;
        cmd->setScissor(scissor);

        cmd->draw(3);

        cmd->endRenderPass();
    }
} // namespace nitro::renderer

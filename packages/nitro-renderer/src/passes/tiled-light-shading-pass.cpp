#include <nitro-renderer/passes/tiled-light-shading-pass.h>

namespace nitro::renderer
{
    TileLightShadingPass::TileLightShadingPass(std::shared_ptr<rhi::RHIDevice> device,
                                               uint32_t width,
                                               uint32_t height,
                                               GBuffer &gBuffer,
                                               const PerFrame<TileLightingComputeResource> &tiledResources,
                                               std::string shaderDir,
                                               bool isMetal)
        : m_device(device),
          m_width(width),
          m_height(height)
    {
        std::vector<rhi::RHIDescriptorBinding> bindings{
            {rhi::RHIDescriptorBinding::Type::UniformBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             2},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             3},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             4},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             5},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             6},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             7},
        };

        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        m_createLightTextureAndRenderPass();

        rhi::PipelineDesc pipelineDesc;

        pipelineDesc.hasDepth = false;
        pipelineDesc.hasStencil = false;
        pipelineDesc.layouts = {m_descriptorLayout};
        pipelineDesc.hasPushConstant = false;
        rhi::RHIBlendDesc blendDesc;
        blendDesc.enabled = true;
        pipelineDesc.colorAttachments = {rhi::PipelineDesc::ColorAttachmentDesc(rhi::TextureDesc::ImageFormat::ColorSRGBA16, blendDesc)};
        pipelineDesc.hasColorAttachment = true;
        std::string shaderPath = shaderDir + "/tiled-light-shading/tiled-light-shading";

        if (isMetal)
        {
            pipelineDesc.shaders.push_back({"vs",
                                            shaderPath + ".metalib",
                                            ShaderStage::Vertex});
            pipelineDesc.shaders.push_back({"fs",
                                            shaderPath + ".metalib",
                                            ShaderStage::Fragment});
        }
        else
        {

            pipelineDesc.shaders.push_back({"main",
                                            shaderPath + ".vert.spv",
                                            ShaderStage::Vertex});
            pipelineDesc.shaders.push_back({"main",
                                            shaderPath + ".frag.spv",
                                            ShaderStage::Fragment});
        }

        m_pipeline = m_device->createPipeline(pipelineDesc);

        m_resources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [&, gBuffer, tiledResources](uint32_t frameIdx)
            {
                TiledLightPassResource resource;
                auto &tileResource = tiledResources.current(frameIdx);
                rhi::BufferDesc uboDesc;
                uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                uboDesc.size = sizeof(TiledLightPassUBO);
                uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

                resource.uniformBuffer = m_device->createBuffer(uboDesc);

                resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

                m_linkDescriptorSet(resource, gBuffer, tileResource);
                return resource;
            });
    }

    void TileLightShadingPass::m_linkDescriptorSet(TiledLightPassResource &resource, const GBuffer &gBuffer, const TileLightingComputeResource &tileResource)
    {
        resource.descriptorSet->writeBuffer(resource.uniformBuffer, 2);
        resource.descriptorSet->writeTexture(gBuffer.depth, 3, ImageLayout::ShaderReadOnly);
        resource.descriptorSet->writeTexture(gBuffer.normal, 4, ImageLayout::ShaderReadOnly);
        resource.descriptorSet->writeBuffer(tileResource.pointLightBuffer, 5);
        resource.descriptorSet->writeBuffer(tileResource.tileLightCountBuffer, 6);
        resource.descriptorSet->writeBuffer(tileResource.tileLightIndicesBuffer, 7);
        resource.descriptorSet->commit();
    }

    TileLightShadingPass::~TileLightShadingPass()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyBuffer(resource.uniformBuffer);
            m_device->destroyDescriptorSet(resource.descriptorSet);
        }
        m_device->destroyTexture(m_lightTexture);
        m_device->destroyRenderPass(m_renderPass);
        m_device->destroyPipeline(m_pipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }
    void TileLightShadingPass::m_createLightTextureAndRenderPass()
    {
        rhi::TextureDesc textureDesc;
        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorSRGBA16;
        textureDesc.size = {m_width, m_height};
        textureDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        m_lightTexture = m_device->createTexture(textureDesc);

        rhi::RenderPassDesc renderPassDesc;
        rhi::RenderPassDesc::Attachment colorAttachment;
        colorAttachment.texture = m_lightTexture;
        colorAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        colorAttachment.store = rhi::RenderPassDesc::StoreOp::Store;
        renderPassDesc.colorAttachments = {colorAttachment};

        renderPassDesc.width = m_width;
        renderPassDesc.height = m_height;

        m_renderPass = m_device->createRenderPass(renderPassDesc);
    }

    void TileLightShadingPass::resize(uint32_t width, uint32_t height, const GBuffer &gBuffer, const PerFrame<TileLightingComputeResource> &tileResources)
    {

        m_width = width;
        m_height = height;
        m_device->destroyTexture(m_lightTexture);
        m_device->destroyRenderPass(m_renderPass);

        m_createLightTextureAndRenderPass();

        for (uint32_t i = 0; i < g_MAX_FRAMES_IN_FLIGHT; i++)
        {
            auto &tileResource = tileResources.current(i);
            auto &resource = m_resources.current(i);

            m_linkDescriptorSet(resource, gBuffer, tileResource);
        }
    }

    void TileLightShadingPass::execute(rhi::RHICommandBuffer *cmd, TiledLightPassUBO ubo)
    {
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        resource.uniformBuffer->upload(&ubo, sizeof(TiledLightPassUBO));

        cmd->beginRenderPass(m_renderPass);
        cmd->bindPipeline(m_pipeline);
        rhi::RHIViewport viewport;
        viewport.width = m_width;
        viewport.height = m_height;

        cmd->setViewPort(viewport);
        rhi::RHIScissor scissor;
        scissor.width = m_width;
        scissor.height = m_height;
        cmd->setScissor(scissor);

        cmd->bindDescriptorSet(resource.descriptorSet, 0);
        cmd->draw(3);
        cmd->endRenderPass();
    }

} // namespace nitro::renderer

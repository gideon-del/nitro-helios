#include <nitro-renderer/passes/tiled-light-shading-pass.h>

namespace nitro::renderer
{
    TileLightShadingPass::TileLightShadingPass(std::shared_ptr<rhi::RHIDevice> device,
                                               uint32_t width,
                                               uint32_t height,
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
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             8},
        };

        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::PipelineDesc pipelineDesc;

        pipelineDesc.hasDepth = false;
        pipelineDesc.hasStencil = false;
        pipelineDesc.layouts = {m_descriptorLayout};
        pipelineDesc.hasPushConstant = false;
        rhi::RHIBlendDesc blendDesc;
        blendDesc.enabled = true;
        pipelineDesc.colorAttachments = {rhi::PipelineDesc::ColorAttachmentDesc(rhi::TextureDesc::ImageFormat::ColorRGBA16, blendDesc)};
        pipelineDesc.hasColorAttachment = true;
        std::string shaderPath = shaderDir + "/tiled-light-shading/tiled-light-shading";
        pipelineDesc.cullMode = PipelineDesc::CullMode::None;

        if (isMetal)
        {
            pipelineDesc.shaders.push_back({"vs",
                                            shaderPath + ".metallib",
                                            ShaderStage::Vertex});
            pipelineDesc.shaders.push_back({"fs",
                                            shaderPath + ".metallib",
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
            [&](uint32_t frameIdx)
            {
                TiledLightPassResource resource;
                rhi::BufferDesc uboDesc;
                uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                uboDesc.size = sizeof(TiledLightPassUBO);
                uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

                resource.uniformBuffer = m_device->createBuffer(uboDesc);

                resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

                return resource;
            });
    }

    void TileLightShadingPass::bindDescriptorSet(const RGResources &rgResources, const TiledLightPassRGResource &rgResourceIds, TiledLightPassResource &resource, rhi::RHIBuffer *pointLightBuffer)
    {
        auto frameidx = m_device->getCurrentFrameIndex();
        resource.lastDepthTexture = rgResources.getTexture(rgResourceIds.depthTexture);
        resource.lastNormalTexture = rgResources.getTexture(rgResourceIds.normalTexture);
        resource.lastPointLightBuffer = pointLightBuffer;
        resource.lastTileLightCountBuffer = rgResources.getBuffer(rgResourceIds.tileLightCount, frameidx);
        resource.lastTileLightIndicesBuffer = rgResources.getBuffer(rgResourceIds.tileLightIndices, frameidx);
        resource.lastTileLightDebugBuffer = rgResources.getBuffer(rgResourceIds.tileLightDebug, frameidx);

        resource.descriptorSet->writeBuffer(resource.uniformBuffer, 2);

        rhi::TextureBinding textureBinding;
        textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
        textureBinding.texture = resource.lastDepthTexture;
        resource.descriptorSet->writeTexture(textureBinding, 3, ImageLayout::ShaderReadOnly);
        textureBinding.texture = resource.lastNormalTexture;
        resource.descriptorSet->writeTexture(textureBinding, 4, ImageLayout::ShaderReadOnly);
        resource.descriptorSet->writeBuffer(resource.lastPointLightBuffer, 5);
        resource.descriptorSet->writeBuffer(resource.lastTileLightCountBuffer, 6);
        resource.descriptorSet->writeBuffer(resource.lastTileLightIndicesBuffer, 7);
        resource.descriptorSet->writeBuffer(resource.lastTileLightDebugBuffer, 8);
        resource.descriptorSet->commit();
    }
    bool TileLightShadingPass::isDescriptorSetStale(const RGResources &rgResources, const TiledLightPassRGResource &rgResourceIds, TiledLightPassResource &resource, rhi::RHIBuffer *pointLightBuffer)
    {

        auto frameidx = m_device->getCurrentFrameIndex();
        return resource.lastDepthTexture != rgResources.getTexture(rgResourceIds.depthTexture) ||
               resource.lastNormalTexture != rgResources.getTexture(rgResourceIds.normalTexture) ||
               resource.lastPointLightBuffer != pointLightBuffer ||
               resource.lastTileLightCountBuffer != rgResources.getBuffer(rgResourceIds.tileLightCount, frameidx) ||
               resource.lastTileLightIndicesBuffer != rgResources.getBuffer(rgResourceIds.tileLightIndices, frameidx) ||
               resource.lastTileLightDebugBuffer != rgResources.getBuffer(rgResourceIds.tileLightDebug, frameidx);
    }

    bool TileLightShadingPass::isRenderPassStale(const RGResources &rgResources, const TiledLightPassRGResource &rgResourceIds)
    {

        return m_lastPointLightTexture != rgResources.getTexture(rgResourceIds.pointLightTexture) || m_renderPass == nullptr;
    };

    void TileLightShadingPass::updateRenderPass(const RGResources &rgResources, const TiledLightPassRGResource &rgResourceIds)
    {

        if (m_renderPass)
        {
            m_device->destroyRenderPass(m_renderPass);
        }
        m_lastPointLightTexture = rgResources.getTexture(rgResourceIds.pointLightTexture);
        rhi::RenderPassDesc renderPassDesc;
        rhi::RenderPassDesc::Attachment colorAttachment;
        colorAttachment.texture = m_lastPointLightTexture;
        colorAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        colorAttachment.store = rhi::RenderPassDesc::StoreOp::Store;
        renderPassDesc.colorAttachments = {colorAttachment};
        renderPassDesc.width = m_width;
        renderPassDesc.height = m_height;

        m_renderPass = m_device->createRenderPass(renderPassDesc);
    };

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

    void TileLightShadingPass::resize(uint32_t width, uint32_t height)
    {

        m_width = width;
        m_height = height;

        m_lastPointLightTexture = nullptr;
        for (auto &r : m_resources)
        {
            r.lastDepthTexture = nullptr;
            r.lastNormalTexture = nullptr;
            r.lastTileLightCountBuffer = nullptr;
            r.lastTileLightIndicesBuffer = nullptr;
            r.lastTileLightDebugBuffer = nullptr;
            r.lastPointLightBuffer = nullptr;
        }
    }

    void TileLightShadingPass::execute(rhi::RHICommandBuffer *cmd, const RGResources &rgResources, const TiledLightPassRGResource &rgResourceIds, Scene &scene, TiledLightPassUBO ubo)
    {
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        auto pointLightBuffer = scene.lightManager()->getPointLightBuffer();
        if (isDescriptorSetStale(rgResources, rgResourceIds, resource, pointLightBuffer))
        {
            bindDescriptorSet(rgResources, rgResourceIds, resource, pointLightBuffer);
        }

        if (isRenderPassStale(rgResources, rgResourceIds))
        {

            updateRenderPass(rgResources, rgResourceIds);
        }
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

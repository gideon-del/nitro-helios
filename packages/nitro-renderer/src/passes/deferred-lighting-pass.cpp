#include <nitro-renderer/passes/deferred-lighting-pass.h>

namespace nitro::renderer
{
    DeferredLightingPass::DeferredLightingPass(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<rhi::RHISwapchain> swapchain, std::vector<rhi::RHITexture *> &cascades, GBuffer &gBuffer, rhi::RHITexture *lightTexture, std::string shaderDir, bool isMetal) : m_device(device), m_swapchain(swapchain), m_isMetal(isMetal)
    {
        std::vector<rhi::RHIDescriptorBinding> mainBindings = {
            {rhi::RHIDescriptorBinding::Type::UniformBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             2}};
        std::vector<rhi::RHIDescriptorBinding> gBufferBindings = {
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             0},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             1},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             2},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             3},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             4},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             5},
        };

        std::vector<rhi::RHIDescriptorBinding> cascadeBindings;

        for (uint32_t i = 0; i < cascades.size(); i++)
        {
            cascadeBindings.push_back({rhi::RHIDescriptorBinding::Type::Sampler,
                                       rhi::RHIDescriptorBinding::ShaderStage::Fragment,
                                       i});
        }

        m_uniformBufferDescriptorLayout = m_device->createDescriptorLayout(mainBindings);
        m_gBufferDescriptorLayout = m_device->createDescriptorLayout(gBufferBindings);
        m_shadowDescriptorLayout = m_device->createDescriptorLayout(cascadeBindings);

        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.hasPushConstant = false;
        pipelineDesc.layouts = {m_uniformBufferDescriptorLayout, m_gBufferDescriptorLayout, m_shadowDescriptorLayout};

        std::string shaderPath = shaderDir + "/deferred-lighting/deferred-lighting";

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

        m_resources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [&, gBuffer, cascades, lightTexture](uint32_t frame)
            {
                DeferredLightingResource resource;
                rhi::BufferDesc uboDesc;
                uboDesc.size = sizeof(DeferredLightingFrameData);
                uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

                resource.uniformBuffer = m_device->createBuffer(uboDesc);
                resource.mainDescriptorSet = m_device->createDescriptorSet(m_uniformBufferDescriptorLayout);
                resource.mainDescriptorSet->writeBuffer(resource.uniformBuffer, 2);
                resource.mainDescriptorSet->commit();

                resource.gBufferDescriptorSet = m_device->createDescriptorSet(m_gBufferDescriptorLayout);

                resource.gBufferDescriptorSet->writeTexture(gBuffer.albedo, 0);
                resource.gBufferDescriptorSet->writeTexture(gBuffer.normal, 1);
                resource.gBufferDescriptorSet->writeTexture(gBuffer.material, 2);
                resource.gBufferDescriptorSet->writeTexture(gBuffer.emissive, 3);
                resource.gBufferDescriptorSet->writeTexture(gBuffer.depth, 4);
                resource.gBufferDescriptorSet->writeTexture(lightTexture, 5);
                resource.gBufferDescriptorSet->commit();

                resource.shadowDescriptorSet = m_device->createDescriptorSet(m_shadowDescriptorLayout);

                for (uint32_t i = 0; i < cascades.size(); i++)
                {
                    resource.shadowDescriptorSet->writeTexture(cascades[i], i);
                }

                resource.shadowDescriptorSet->commit();

                return resource;
            });
    };

    DeferredLightingPass::~DeferredLightingPass()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyBuffer(resource.uniformBuffer);
        }

        m_device->destroyPipeline(m_pipeline);
    }

    void DeferredLightingPass::execute(rhi::RHICommandBuffer *cmd, DeferredLightingFrameData frameData)
    {
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());
        cmd->bindPipeline(m_pipeline);
        resource.uniformBuffer->upload(&frameData, sizeof(DeferredLightingFrameData));
        RHIViewScale swapchainViewScale = m_swapchain->getViewScale();
        rhi::RHIViewport viewport;
        viewport.width = m_swapchain->getWidth() * swapchainViewScale.x;
        viewport.height = m_swapchain->getHeight() * swapchainViewScale.y;

        cmd->setViewPort(viewport);

        rhi::RHIScissor scissor;
        scissor.width = m_swapchain->getWidth() * swapchainViewScale.x;
        scissor.height = m_swapchain->getHeight() * swapchainViewScale.y;
        cmd->setScissor(scissor);
        cmd->bindDescriptorSet(resource.mainDescriptorSet, 0);
        cmd->bindDescriptorSet(resource.gBufferDescriptorSet, 1);
        cmd->bindDescriptorSet(resource.shadowDescriptorSet, 2);
        cmd->draw(3);
    }

    void DeferredLightingPass::recreate(GBuffer &gBuffer, rhi::RHITexture *lightTexture)
    {
        for (auto &resource : m_resources)
        {
            resource.gBufferDescriptorSet->writeTexture(gBuffer.albedo, 0);
            resource.gBufferDescriptorSet->writeTexture(gBuffer.normal, 1);
            resource.gBufferDescriptorSet->writeTexture(gBuffer.material, 2);
            resource.gBufferDescriptorSet->writeTexture(gBuffer.emissive, 3);
            resource.gBufferDescriptorSet->writeTexture(gBuffer.depth, 4);
            resource.gBufferDescriptorSet->writeTexture(lightTexture, 5);
            resource.gBufferDescriptorSet->commit();
        }
    };

} // namespace nitro::renderer

#include <nitro-renderer/passes/forward-lighting-pass.h>
#include <nitro-geometry/push-constant.h>
#include <nitro-geometry/vertex.h>
namespace nitro::renderer
{
    ForwardLightingPass::ForwardLightingPass(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<rhi::RHISwapchain> swapchain, std::vector<rhi::RHITexture *> &cascades, std::string shaderDir, bool isMetal) : m_device(device), m_swapchain(swapchain)
    {
        std::vector<rhi::RHIDescriptorBinding> mainBindings = {{rhi::RHIDescriptorBinding::Type::UniformBuffer,
                                                                rhi::RHIDescriptorBinding::ShaderStage::Both,
                                                                2}};
        std::vector<rhi::RHIDescriptorBinding> cascadeBindings;

        for (uint32_t i = 0; i < cascades.size(); i++)
        {
            cascadeBindings.push_back({rhi::RHIDescriptorBinding::Type::Sampler,
                                       rhi::RHIDescriptorBinding::ShaderStage::Fragment,
                                       i});
        }
        m_mainDescriptorLayout = m_device->createDescriptorLayout(mainBindings);
        m_shadowDescriptorLayout = m_device->createDescriptorLayout(cascadeBindings);
        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.hasPushConstant = true;
        pipelineDesc.pushConstantSize = sizeof(geometry::PushConstant);
        pipelineDesc.layouts = {m_mainDescriptorLayout, m_shadowDescriptorLayout};
        pipelineDesc.vertexLayout = geometry::Vertex::getVertexLayout();

        std::string shaderPath = shaderDir + "/forward-lighting/forward-lighting";

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

        rhi::BufferDesc uboDesc;
        uboDesc.size = sizeof(FrameData);
        uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
        uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

        for (int i = 0; i < FrameResource::MAX_FRAME_RESOURCES; i++)
        {
            FrameResource frameResource(m_device.get());

            rhi::RHIBuffer *uboBuffer = m_device->createBuffer(uboDesc);

            frameResource.setBuffer(FrameResourceId::ForwardLightingUniformBuffer, uboBuffer);

            rhi::RHIDescriptorSet *uboDescriptorSet = m_device->createDescriptorSet(m_mainDescriptorLayout);

            uboDescriptorSet->writeBuffer(uboBuffer, 2);
            uboDescriptorSet->commit();
            frameResource.setDescriptorSet(FrameResourceId::ForwardLightingMainDescriptorSet, uboDescriptorSet);

            rhi::RHIDescriptorSet *shadowDescriptorSet = m_device->createDescriptorSet(m_shadowDescriptorLayout);

            for (uint32_t j = 0; j < cascades.size(); j++)
            {
                shadowDescriptorSet->writeTexture(cascades[j], j);
            }
            shadowDescriptorSet->commit();
            frameResource.setDescriptorSet(FrameResourceId::ForwardLightingShadowDescriptorSet, shadowDescriptorSet);

            m_frameResources.push_back(frameResource);
        }
    }

    ForwardLightingPass::~ForwardLightingPass()
    {
        for (auto &frameResource : m_frameResources)
        {
            m_device->destroyBuffer(frameResource.getBuffer(FrameResourceId::ForwardLightingUniformBuffer));
        }
        m_device->destroyPipeline(m_pipeline);
    }

    void ForwardLightingPass::execute(rhi::RHICommandBuffer *cmd, Scene &scene, FrameData frameData)
    {

        uint32_t frameIdx = m_device->getCurrentFrameIndex();
        m_frameResources[frameIdx].getBuffer(FrameResourceId::ForwardLightingUniformBuffer)->upload(&frameData, sizeof(FrameData));

        cmd->bindPipeline(m_pipeline);
        cmd->bindDescriptorSet(m_frameResources[frameIdx].getDescriptorSet(FrameResourceId::ForwardLightingMainDescriptorSet), 0);
        cmd->bindDescriptorSet(m_frameResources[frameIdx].getDescriptorSet(FrameResourceId::ForwardLightingShadowDescriptorSet), 1);

        RHIViewport mainViewPort;
        mainViewPort.width = m_swapchain->getWidth();
        mainViewPort.height = m_swapchain->getHeight();
        cmd->setViewPort(mainViewPort);
        RHIScissor mainScissor;
        mainScissor.width = m_swapchain->getWidth();
        mainScissor.height = m_swapchain->getHeight();
        cmd->setScissor(mainScissor);

        scene.draw(cmd);
    }
} // namespace nitro::renderer

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
        pipelineDesc.depthTest = true;
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

        m_resources.create(g_MAX_FRAMES_IN_FLIGHT,
                           [&, cascades](uint32_t frame)
                           {
                               ForwardLightingResource resource;
                               rhi::BufferDesc uboDesc;
                               uboDesc.size = sizeof(FrameData);
                               uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                               uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

                               resource.uniformBuffer = m_device->createBuffer(uboDesc);
                               resource.mainDescriptorSet = m_device->createDescriptorSet(m_mainDescriptorLayout);

                               resource.mainDescriptorSet->writeBuffer(resource.uniformBuffer, 2);
                               resource.mainDescriptorSet->commit();

                               resource.shadowDescriptorSet = m_device->createDescriptorSet(m_shadowDescriptorLayout);
                               for (uint32_t j = 0; j < cascades.size(); j++)
                               {
                                   resource.shadowDescriptorSet->writeTexture(cascades[j], j);
                               }
                               resource.shadowDescriptorSet->commit();

                               return resource;
                           });
    }

    ForwardLightingPass::~ForwardLightingPass()
    {
        for (auto &frameResource : m_resources)
        {
            m_device->destroyBuffer(frameResource.uniformBuffer);
        }
        m_device->destroyPipeline(m_pipeline);
    }

    void ForwardLightingPass::execute(rhi::RHICommandBuffer *cmd, Scene &scene, FrameData frameData)
    {

        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());
        resource.uniformBuffer->upload(&frameData, sizeof(FrameData));

        cmd->bindPipeline(m_pipeline);
        cmd->bindDescriptorSet(resource.mainDescriptorSet, 0);
        cmd->bindDescriptorSet(resource.shadowDescriptorSet, 1);

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

#include <nitro-renderer/passes/depth-prepass.h>
#include <nitro-geometry/push-constant.h>
#include <nitro-geometry/vertex.h>
namespace nitro::renderer
{
    DepthPrepass::DepthPrepass(std::shared_ptr<rhi::RHIDevice> device,
                               uint32_t width,
                               uint32_t height,
                               std::string shaderDir,
                               bool isMetal) : m_device(device), m_width(width), m_height(height)
    {

        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::UniformBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Vertex,
             2},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Vertex,
             3},

        };

        m_descriptorLayout = m_device->createDescriptorLayout(binding);
        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.depthTest = rhi::CompareOp::Less;
        pipelineDesc.depthWrite = true;
        pipelineDesc.hasColorAttachment = false;
        pipelineDesc.layouts = {m_descriptorLayout};
        pipelineDesc.vertexLayout = geometry::Vertex::getVertexLayout();
        pipelineDesc.depthAttachmentFormat = rhi::TextureDesc::ImageFormat::Depth32Float;
        pipelineDesc.hasDepth = true;
        pipelineDesc.hasStencil = false;
        pipelineDesc.hasPushConstant = false;
        std::string shaderPath = shaderDir + "/depth-prepass/depth-prepass";

        if (isMetal)
        {
            pipelineDesc.shaders.push_back({"vs", shaderPath + ".metallib", rhi::ShaderStage::Vertex});
        }
        else
        {
            pipelineDesc.shaders.push_back({"main", shaderPath + ".vert.spv", rhi::ShaderStage::Vertex});
        }

        m_pipeline = m_device->createPipeline(pipelineDesc);

        rhi::RenderPassDesc renderPassDesc;

        m_resources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [&](uint32_t frame)
            {
                DepthResource resource;
                rhi::BufferDesc uboDesc;
                uboDesc.size = sizeof(DepthPrePassCamera);
                uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

                resource.uniformBuffer = m_device->createBuffer(uboDesc);

                resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

                return resource;
            });
    }

    DepthPrepass::~DepthPrepass()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyBuffer(resource.uniformBuffer);
        }

        m_device->destroyPipeline(m_pipeline);
    }
    void DepthPrepass::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    }

    void DepthPrepass::bindResources(const RGResources &resource, RGTextureID depth)
    {
        auto depthTexture = resource.getTexture(depth);

        if (m_renderPass != nullptr)
        {
            m_device->destroyRenderPass(m_renderPass);
        }

        rhi::RenderPassDesc renderPassDesc;

        rhi::RenderPassDesc::Attachment depthAttachment;
        depthAttachment.texture = depthTexture;
        depthAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        depthAttachment.store = rhi::RenderPassDesc::StoreOp::Store;

        renderPassDesc.depthAttachment = &depthAttachment;
        renderPassDesc.width = m_width;
        renderPassDesc.height = m_height;

        m_renderPass = m_device->createRenderPass(renderPassDesc);
    }

    void DepthPrepass::bindSceneBuffers(Scene &scene)
    {
        m_lastMeshInstanceBuffer = scene.meshManager->instanceBuffer();
        for (auto &resource : m_resources)
        {
            resource.descriptorSet->writeBuffer(resource.uniformBuffer, 2);
            resource.descriptorSet->writeBuffer(m_lastMeshInstanceBuffer, 3);
            resource.descriptorSet->commit();
        }
    }

    bool DepthPrepass::isSceneBuffersStale(Scene &scene)
    {
        return m_lastMeshInstanceBuffer != scene.meshManager->instanceBuffer();
    }

    void DepthPrepass::execute(rhi::RHICommandBuffer *cmd, Scene &scene, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer, DepthPrePassCamera camera)
    {

        if (isSceneBuffersStale(scene))
        {
            bindSceneBuffers(scene);
        }
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());
        cmd->beginRenderPass(m_renderPass);
        cmd->bindPipeline(m_pipeline);
        resource.uniformBuffer->upload(&camera, sizeof(DepthPrePassCamera));
        cmd->bindDescriptorSet(resource.descriptorSet, 0);
        rhi::RHIViewport viewport;
        viewport.width = m_width;
        viewport.height = m_height;

        rhi::RHIScissor scissor;
        scissor.width = m_width;
        scissor.height = m_height;

        cmd->setViewPort(viewport);
        cmd->setScissor(scissor);

        scene.draw(cmd, drawCommandBuffer, drawCountBuffer);

        cmd->endRenderPass();
    }
} // namespace nitro::renderer

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

        rhi::TextureDesc textureDesc;
        textureDesc.format = rhi::TextureDesc::ImageFormat::Depth32FloatStencil8;
        textureDesc.usage = rhi::TextureDesc::Usage::DepthStencil | rhi::TextureDesc::Usage::ShaderRead;
        textureDesc.size = {m_width, m_height};

        m_depthTexture = m_device->createTexture(textureDesc);

        std::vector<rhi::RHIDescriptorBinding> binding{{rhi::RHIDescriptorBinding::Type::UniformBuffer,
                                                        rhi::RHIDescriptorBinding::ShaderStage::Vertex,
                                                        2}};

        m_descriptorLayout = m_device->createDescriptorLayout(binding);
        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.depthTest = rhi::CompareOp::Less;
        pipelineDesc.depthWrite = true;
        pipelineDesc.hasColorAttachment = false;
        pipelineDesc.pushConstantSize = sizeof(geometry::PushConstant);
        pipelineDesc.layouts = {m_descriptorLayout};
        pipelineDesc.vertexLayout = geometry::Vertex::getVertexLayout();
        pipelineDesc.depthAttachmentFormat = rhi::TextureDesc::ImageFormat::Depth32FloatStencil8;
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

        rhi::RenderPassDesc::Attachment depthAttachment;
        depthAttachment.texture = m_depthTexture;
        depthAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        depthAttachment.store = rhi::RenderPassDesc::StoreOp::Store;
        depthAttachment.hasStencil = true;
        depthAttachment.stencilLoad = rhi::RenderPassDesc::LoadOp::Clear;
        depthAttachment.stencilStore = rhi::RenderPassDesc::StoreOp::Store;
        depthAttachment.clearStencil = 0;
        renderPassDesc.depthAttachment = &depthAttachment;
        renderPassDesc.width = m_width;
        renderPassDesc.height = m_height;

        m_renderPass = m_device->createRenderPass(renderPassDesc);

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

                resource.descriptorSet->writeBuffer(resource.uniformBuffer, 2);
                resource.descriptorSet->commit();

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
        m_device->destroyTexture(m_depthTexture);
    }
    void DepthPrepass::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;

        m_device->destroyTexture(m_depthTexture);

        rhi::TextureDesc textureDesc;
        textureDesc.format = rhi::TextureDesc::ImageFormat::Depth32FloatStencil8;
        textureDesc.usage = rhi::TextureDesc::Usage::DepthStencil | rhi::TextureDesc::Usage::ShaderRead;
        textureDesc.size = {m_width, m_height};

        m_depthTexture = m_device->createTexture(textureDesc);

        rhi::RenderPassDesc renderPassDesc;

        rhi::RenderPassDesc::Attachment depthAttachment;
        depthAttachment.texture = m_depthTexture;
        depthAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        depthAttachment.store = rhi::RenderPassDesc::StoreOp::Store;
        depthAttachment.hasStencil = true;
        depthAttachment.stencilLoad = rhi::RenderPassDesc::LoadOp::Clear;
        depthAttachment.stencilStore = rhi::RenderPassDesc::StoreOp::Store;
        depthAttachment.clearStencil = 0;
        renderPassDesc.depthAttachment = &depthAttachment;
        renderPassDesc.width = m_width;
        renderPassDesc.height = m_height;

        m_renderPass = m_device->createRenderPass(renderPassDesc);
    }

    void DepthPrepass::execute(rhi::RHICommandBuffer *cmd, Scene &scene, DepthPrePassCamera camera)
    {

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

        scene.draw(cmd);

        cmd->endRenderPass();
    }
} // namespace nitro::renderer

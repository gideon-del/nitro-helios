#include <nitro-renderer/passes/geometry-pass.h>
#include <nitro-geometry/push-constant.h>
#include <nitro-geometry/vertex.h>
namespace nitro::renderer
{
    GeometryPass::GeometryPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal)
        : m_width(width),
          m_height(height),
          m_device(device)

    {

        std::vector<rhi::RHIDescriptorBinding> bindings = {
            {RHIDescriptorBinding::Type::UniformBuffer, RHIDescriptorBinding::ShaderStage::Vertex, 2},
            {RHIDescriptorBinding::Type::StorageBuffer, RHIDescriptorBinding::ShaderStage::Both, 3},
            {RHIDescriptorBinding::Type::StorageBuffer, RHIDescriptorBinding::ShaderStage::Fragment, 4},
            {RHIDescriptorBinding::Type::Texture, RHIDescriptorBinding::ShaderStage::Fragment, 5, true, 4096},
            {RHIDescriptorBinding::Type::SingleSampler, RHIDescriptorBinding::ShaderStage::Fragment, 6},
        };
        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.hasColorAttachment = true;
        pipelineDesc.colorAttachments = {rhi::TextureDesc::ImageFormat::ColorRGBA8, rhi::TextureDesc::ImageFormat::ColorRG8U, rhi::TextureDesc::ImageFormat::ColorRGBA8,
                                         rhi::TextureDesc::ImageFormat::ColorRGBA16};
        pipelineDesc.depthWrite = false;
        pipelineDesc.depthTest = rhi::CompareOp::Equal;
        std::string shaderPath = shaderDir + "/geometry/geometry";

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

        pipelineDesc.layouts = {m_descriptorLayout};
        pipelineDesc.vertexLayout = geometry::Vertex::getVertexLayout();
        pipelineDesc.hasPushConstant = false;

        pipelineDesc.depthAttachmentFormat = rhi::TextureDesc::ImageFormat::Depth32Float;
        pipelineDesc.cullMode = PipelineDesc::CullMode::Back;

        m_pipeline = m_device->createPipeline(pipelineDesc);

        m_resources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [&](uint32_t frame)
            {
                rhi::BufferDesc bufferDesc;
                bufferDesc.size = sizeof(GeometryCameraBuffer);
                bufferDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                bufferDesc.usage = rhi::BufferDesc::Usage::Uniform;
                GeometryPassResource resource;
                resource.uniformBuffer = m_device->createBuffer(bufferDesc);
                resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

                return resource;
            });
    }

    void GeometryPass::execute(rhi::RHICommandBuffer *cmd, GeometryCameraBuffer geometryCamera, Scene &scene, LightingSettings &settings, rhi::RHIBuffer *drawCommandBuffer, rhi::RHIBuffer *drawCountBuffer)
    {

        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());
        if (isSceneBuffersStale(scene, resource))
        {
            bindSceneBuffers(scene, resource);
        }

        cmd->beginRenderPass(m_renderPass);
        cmd->bindPipeline(m_pipeline);
        RHIViewport viewport;
        viewport.width = m_width;
        viewport.height = m_height;
        cmd->setViewPort(viewport);
        RHIScissor scissor;
        scissor.width = m_width;
        scissor.height = m_height;
        cmd->setScissor(scissor);
        resource.uniformBuffer->upload(&geometryCamera, sizeof(GeometryCameraBuffer));
        cmd->bindDescriptorSet(resource.descriptorSet, 0);
        scene.draw(cmd, drawCommandBuffer, drawCountBuffer);
        cmd->endRenderPass();
    };

    void GeometryPass::bindSceneBuffers(Scene &scene, GeometryPassResource &resource)
    {
        resource.lastMeshInstanceBuffer = scene.meshManager->instanceBuffer();
        resource.lastMaterialBuffer = scene.materialManager->getMaterialBuffer();
        resource.descriptorSet->writeBuffer(resource.uniformBuffer, 2);
        resource.descriptorSet->writeBuffer(resource.lastMeshInstanceBuffer, 3);
        resource.descriptorSet->writeBuffer(resource.lastMaterialBuffer, 4);
        resource.descriptorSet->writeBindlessTextures(scene.materialManager->getTextures(), 5);
        resource.descriptorSet->writeSampler(m_device->defaultSamplers().anisotropicRepeat, 6);
        resource.descriptorSet->commit();
    }

    bool GeometryPass::isSceneBuffersStale(Scene &scene, GeometryPassResource &resource)
    {
        return resource.lastMeshInstanceBuffer != scene.meshManager->instanceBuffer() || resource.lastMaterialBuffer != scene.materialManager->getMaterialBuffer();
    }
    GeometryPass::~GeometryPass()
    {
        for (auto &frameResource : m_resources)
        {
            m_device->destroyBuffer(frameResource.uniformBuffer);
            m_device->destroyDescriptorSet(frameResource.descriptorSet);
        }

        m_device->destroyPipeline(m_pipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void GeometryPass::bindResources(const RGResources &resources, const GBuffer &gBuffer)
    {
        if (m_renderPass)
        {
            m_device->destroyRenderPass(m_renderPass);
        }

        rhi::RenderPassDesc renderPassDesc;
        renderPassDesc.width = m_width;
        renderPassDesc.height = m_height;
        rhi::RenderPassDesc::Attachment colorAttachment;
        colorAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        colorAttachment.store = rhi::RenderPassDesc::StoreOp::Store;
        colorAttachment.texture = resources.getTexture(gBuffer.albedo);
        renderPassDesc.colorAttachments.push_back(colorAttachment);
        colorAttachment.texture = resources.getTexture(gBuffer.normal);
        renderPassDesc.colorAttachments.push_back(colorAttachment);
        colorAttachment.texture = resources.getTexture(gBuffer.material);
        renderPassDesc.colorAttachments.push_back(colorAttachment);
        colorAttachment.texture = resources.getTexture(gBuffer.emissive);
        renderPassDesc.colorAttachments.push_back(colorAttachment);

        rhi::RenderPassDesc::Attachment depthAttachment;
        depthAttachment.load = rhi::RenderPassDesc::LoadOp::Load;
        depthAttachment.store = rhi::RenderPassDesc::StoreOp::DontCare;
        depthAttachment.texture = resources.getTexture(gBuffer.depth);
        depthAttachment.depthWrite = false;
        depthAttachment.stencilWrite = false;

        renderPassDesc.depthAttachment = &depthAttachment;

        m_renderPass = m_device->createRenderPass(renderPassDesc);
    };

    void GeometryPass::resize(uint32_t width, uint32_t height)
    {

        m_width = width;
        m_height = height;
    }
}
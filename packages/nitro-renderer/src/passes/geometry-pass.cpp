#include <nitro-renderer/passes/geometry-pass.h>
#include <nitro-geometry/push-constant.h>
#include <nitro-geometry/vertex.h>
namespace nitro::renderer
{
    GeometryPass::GeometryPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal) : width(width), height(height), m_device(device)
    {
        rhi::TextureDesc colorAttachmentDesc;
        colorAttachmentDesc.size = {width, height};
        colorAttachmentDesc.usage = rhi::TextureDesc::Usage::RenderTarget |
                                    rhi::TextureDesc::Usage::ShaderRead;
        colorAttachmentDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;
        gBuffer.albedo = m_device->createTexture(colorAttachmentDesc);
        colorAttachmentDesc.format = rhi::TextureDesc::ImageFormat::ColorSRGBA16;
        gBuffer.normal = m_device->createTexture(colorAttachmentDesc);
        colorAttachmentDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;
        gBuffer.material = m_device->createTexture(colorAttachmentDesc);
        colorAttachmentDesc.format = rhi::TextureDesc::ImageFormat::ColorSRGBA16;
        gBuffer.emissive = m_device->createTexture(colorAttachmentDesc);

        rhi::TextureDesc depthAttachmentDesc;
        depthAttachmentDesc.size = {width, height};
        depthAttachmentDesc.usage = rhi::TextureDesc::Usage::DepthStencil |
                                    rhi::TextureDesc::Usage::ShaderRead;
        depthAttachmentDesc.format = rhi::TextureDesc::ImageFormat::Depth32Float;
        depthAttachmentDesc.sampler = rhi::TextureDesc::Sampler::Sampler2D;

        gBuffer.depth = m_device->createTexture(depthAttachmentDesc);

        rhi::RenderPassDesc renderPassDesc;
        renderPassDesc.width = width;
        renderPassDesc.height = height;
        rhi::RenderPassDesc::Attachment colorAttachment;
        colorAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        colorAttachment.store = rhi::RenderPassDesc::StoreOp::Store;
        colorAttachment.texture = gBuffer.albedo;
        renderPassDesc.colorAttachments.push_back(colorAttachment);
        colorAttachment.texture = gBuffer.normal;
        renderPassDesc.colorAttachments.push_back(colorAttachment);
        colorAttachment.texture = gBuffer.material;
        renderPassDesc.colorAttachments.push_back(colorAttachment);
        colorAttachment.texture = gBuffer.emissive;
        renderPassDesc.colorAttachments.push_back(colorAttachment);

        rhi::RenderPassDesc::Attachment depthAttachment;
        depthAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        depthAttachment.store = rhi::RenderPassDesc::StoreOp::Store;
        depthAttachment.texture = gBuffer.depth;

        renderPassDesc.depthAttachment = &depthAttachment;

        m_renderPass = m_device->createRenderPass(renderPassDesc);

        std::vector<rhi::RHIDescriptorBinding> bindings = {
            {RHIDescriptorBinding::Type::UniformBuffer, RHIDescriptorBinding::ShaderStage::Vertex, 2}};
        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.hasColorAttachment = true;
        pipelineDesc.colorAttachments = {rhi::TextureDesc::ImageFormat::ColorRGBA8, rhi::TextureDesc::ImageFormat::ColorSRGBA16, rhi::TextureDesc::ImageFormat::ColorRGBA8,
                                         rhi::TextureDesc::ImageFormat::ColorSRGBA16};
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
        pipelineDesc.hasPushConstant = true;
        pipelineDesc.pushConstantSize = sizeof(geometry::PushConstant);
        pipelineDesc.depthTest = true;
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
                resource.descriptorSet->writeBuffer(resource.uniformBuffer, 2);
                resource.descriptorSet->commit();
                return resource;
            });
    }

    void GeometryPass::execute(rhi::RHICommandBuffer *cmd, GeometryCameraBuffer geometryCamera, Scene &scene)
    {
        uint32_t frameIdx = m_device->getCurrentFrameIndex();
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());
        cmd->beginRenderPass(m_renderPass);
        cmd->bindPipeline(m_pipeline);
        RHIViewport viewport;
        viewport.width = width;
        viewport.height = height;
        cmd->setViewPort(viewport);
        RHIScissor scissor;
        scissor.width = width;
        scissor.height = height;
        cmd->setScissor(scissor);
        resource.uniformBuffer->upload(&geometryCamera, sizeof(GeometryCameraBuffer));
        cmd->bindDescriptorSet(resource.descriptorSet, 0);

        scene.draw(cmd);

        cmd->endRenderPass();
    };

    GeometryPass::~GeometryPass()
    {
        for (auto &frameResource : m_resources)
        {
            m_device->destroyBuffer(frameResource.uniformBuffer);
        }

        m_device->destroyPipeline(m_pipeline);
        m_device->destroyTexture(gBuffer.albedo);
        m_device->destroyTexture(gBuffer.depth);
        m_device->destroyTexture(gBuffer.normal);
        m_device->destroyTexture(gBuffer.material);
        m_device->destroyTexture(gBuffer.emissive);
    }

    void GeometryPass::resize(uint32_t newWidth, uint32_t newHeight)
    {

        width = newWidth;
        height = newHeight;
        m_device->destroyTexture(gBuffer.albedo);
        m_device->destroyTexture(gBuffer.depth);
        m_device->destroyTexture(gBuffer.normal);
        m_device->destroyTexture(gBuffer.material);
        m_device->destroyTexture(gBuffer.emissive);

        rhi::TextureDesc colorAttachmentDesc;
        colorAttachmentDesc.size = {width, height};
        colorAttachmentDesc.usage = rhi::TextureDesc::Usage::RenderTarget |
                                    rhi::TextureDesc::Usage::ShaderRead;
        colorAttachmentDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;
        gBuffer.albedo = m_device->createTexture(colorAttachmentDesc);
        colorAttachmentDesc.format = rhi::TextureDesc::ImageFormat::ColorSRGBA16;
        gBuffer.normal = m_device->createTexture(colorAttachmentDesc);
        colorAttachmentDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;
        gBuffer.material = m_device->createTexture(colorAttachmentDesc);
        colorAttachmentDesc.format = rhi::TextureDesc::ImageFormat::ColorSRGBA16;
        gBuffer.emissive = m_device->createTexture(colorAttachmentDesc);

        rhi::TextureDesc depthAttachmentDesc;
        depthAttachmentDesc.size = {width, height};
        depthAttachmentDesc.usage = rhi::TextureDesc::Usage::DepthStencil |
                                    rhi::TextureDesc::Usage::ShaderRead;
        depthAttachmentDesc.format = rhi::TextureDesc::ImageFormat::Depth32Float;
        depthAttachmentDesc.sampler = rhi::TextureDesc::Sampler::Sampler2D;

        gBuffer.depth = m_device->createTexture(depthAttachmentDesc);

        rhi::RenderPassDesc renderPassDesc;
        renderPassDesc.width = width;
        renderPassDesc.height = height;
        rhi::RenderPassDesc::Attachment colorAttachment;
        colorAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        colorAttachment.store = rhi::RenderPassDesc::StoreOp::Store;
        colorAttachment.texture = gBuffer.albedo;
        renderPassDesc.colorAttachments.push_back(colorAttachment);
        colorAttachment.texture = gBuffer.normal;
        renderPassDesc.colorAttachments.push_back(colorAttachment);
        colorAttachment.texture = gBuffer.material;
        renderPassDesc.colorAttachments.push_back(colorAttachment);
        colorAttachment.texture = gBuffer.emissive;
        renderPassDesc.colorAttachments.push_back(colorAttachment);

        rhi::RenderPassDesc::Attachment depthAttachment;
        depthAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        depthAttachment.store = rhi::RenderPassDesc::StoreOp::Store;
        depthAttachment.texture = gBuffer.depth;

        renderPassDesc.depthAttachment = &depthAttachment;

        m_renderPass = m_device->createRenderPass(renderPassDesc);
    }
}
#include <nitro-renderer/passes/geometry-pass.h>
#include <nitro-geometry/push-constant.h>
#include <nitro-geometry/vertex.h>
namespace nitro::renderer
{
    GeometryPass::GeometryPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, rhi::RHITexture *depthTexture, std::string shaderDir, bool isMetal) : m_width(width), m_height(height), m_device(device)
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

        gBuffer.depth = depthTexture;

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
        depthAttachment.load = rhi::RenderPassDesc::LoadOp::Load;
        depthAttachment.store = rhi::RenderPassDesc::StoreOp::DontCare;
        depthAttachment.hasStencil = true;
        depthAttachment.stencilLoad = rhi::RenderPassDesc::LoadOp::Clear;
        depthAttachment.stencilStore = rhi::RenderPassDesc::StoreOp::Store;
        depthAttachment.clearStencil = 0;
        depthAttachment.texture = depthTexture;

        renderPassDesc.depthAttachment = &depthAttachment;

        m_renderPass = m_device->createRenderPass(renderPassDesc);

        std::vector<rhi::RHIDescriptorBinding> bindings = {
            {RHIDescriptorBinding::Type::UniformBuffer, RHIDescriptorBinding::ShaderStage::Vertex, 2}};
        std::vector<rhi::RHIDescriptorBinding> materialBindings = {
            {RHIDescriptorBinding::Type::Sampler, RHIDescriptorBinding::ShaderStage::Fragment, 0},
            {RHIDescriptorBinding::Type::Sampler, RHIDescriptorBinding::ShaderStage::Fragment, 1},
            {RHIDescriptorBinding::Type::Sampler, RHIDescriptorBinding::ShaderStage::Fragment, 2},
            {RHIDescriptorBinding::Type::Sampler, RHIDescriptorBinding::ShaderStage::Fragment, 3},
            {RHIDescriptorBinding::Type::Sampler, RHIDescriptorBinding::ShaderStage::Fragment, 4},
        };
        m_descriptorLayout = m_device->createDescriptorLayout(bindings);
        m_materialDescriptorLayout = m_device->createDescriptorLayout(materialBindings);
        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.hasColorAttachment = true;
        pipelineDesc.colorAttachments = {rhi::TextureDesc::ImageFormat::ColorRGBA8, rhi::TextureDesc::ImageFormat::ColorSRGBA16, rhi::TextureDesc::ImageFormat::ColorRGBA8,
                                         rhi::TextureDesc::ImageFormat::ColorSRGBA16};
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

        pipelineDesc.layouts = {m_descriptorLayout, m_materialDescriptorLayout};
        pipelineDesc.vertexLayout = geometry::Vertex::getVertexLayout();
        pipelineDesc.hasPushConstant = true;
        pipelineDesc.pushConstantSize = sizeof(RenderObjectPushConstant);
        pipelineDesc.depthAttachmentFormat = rhi::TextureDesc::ImageFormat::Depth32FloatStencil8;
        pipelineDesc.hasStencil = true;
        pipelineDesc.cullMode = PipelineDesc::CullMode::None;

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
        auto createSolidTexture = [&](glm::vec4 color)
        {
            rhi::TextureDesc textureDesc;
            textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead;
            textureDesc.size = {1, 1};
            textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;
            uint8_t bytes[4] = {
                uint8_t(color.r * 255.0f),
                uint8_t(color.g * 255.0f),
                uint8_t(color.b * 255.0f),
                uint8_t(color.a * 255.0f)};
            textureDesc.initialData = bytes;
            textureDesc.sampler = rhi::TextureDesc::Sampler::Sampler2D;

            return m_device->createTexture(textureDesc);
        };

        m_defaultTextures.baseColor = createSolidTexture({1.0f, 1.0f, 1.0f, 1.0f});
        m_defaultTextures.normal = createSolidTexture({0.5f, 0.5f, 1.0f, 1.0f});
        m_defaultTextures.metallicRoughness = createSolidTexture({0.0f, 0.5f, 0.0f, 1.0f});
        m_defaultTextures.ao = createSolidTexture({1.0f, 1.0f, 1.0f, 1.0f});
        m_defaultTextures.emissive = createSolidTexture({0.0f, 0.0f, 0.0f, 1.0f});

        m_defaultMaterialDescriptorSet = m_device->createDescriptorSet(m_materialDescriptorLayout);

        m_defaultMaterialDescriptorSet->writeTexture(m_defaultTextures.baseColor, 0, ImageLayout::ShaderReadOnly);
        m_defaultMaterialDescriptorSet->writeTexture(m_defaultTextures.normal, 1, ImageLayout::ShaderReadOnly);
        m_defaultMaterialDescriptorSet->writeTexture(m_defaultTextures.metallicRoughness, 2, ImageLayout::ShaderReadOnly);
        m_defaultMaterialDescriptorSet->writeTexture(m_defaultTextures.ao, 3, ImageLayout::ShaderReadOnly);
        m_defaultMaterialDescriptorSet->writeTexture(m_defaultTextures.emissive, 4, ImageLayout::ShaderReadOnly);
        m_defaultMaterialDescriptorSet->commit();
    }

    void GeometryPass::execute(rhi::RHICommandBuffer *cmd, GeometryCameraBuffer geometryCamera, Scene &scene, LightingSettings &settings)
    {
        uint32_t frameIdx = m_device->getCurrentFrameIndex();
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());
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

        for (auto &obj : scene.objects)
        {
            obj.draw(cmd, nullptr, 0, m_defaultMaterialDescriptorSet);
        }

        cmd->endRenderPass();
    };

    GeometryPass::~GeometryPass()
    {
        for (auto &frameResource : m_resources)
        {
            m_device->destroyBuffer(frameResource.uniformBuffer);
            m_device->destroyDescriptorSet(frameResource.descriptorSet);
        }

        m_device->destroyPipeline(m_pipeline);
        m_device->destroyTexture(gBuffer.albedo);
        m_device->destroyTexture(gBuffer.normal);
        m_device->destroyTexture(gBuffer.material);
        m_device->destroyTexture(gBuffer.emissive);
        m_device->destroyDescriptorSet(m_defaultMaterialDescriptorSet);
        m_device->destroyDescriptorLayout(m_materialDescriptorLayout);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void GeometryPass::resize(uint32_t width, uint32_t height, rhi::RHITexture *depthTexture)
    {

        m_width = width;
        m_height = height;
        m_device->destroyTexture(gBuffer.albedo);
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

        gBuffer.depth = depthTexture;

        rhi::RenderPassDesc renderPassDesc;
        renderPassDesc.width = m_width;
        renderPassDesc.height = m_height;
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
        depthAttachment.load = rhi::RenderPassDesc::LoadOp::Load;
        depthAttachment.store = rhi::RenderPassDesc::StoreOp::DontCare;
        depthAttachment.hasStencil = true;
        depthAttachment.stencilLoad = rhi::RenderPassDesc::LoadOp::Clear;
        depthAttachment.stencilStore = rhi::RenderPassDesc::StoreOp::Store;
        depthAttachment.clearStencil = 0;
        depthAttachment.texture = gBuffer.depth;

        renderPassDesc.depthAttachment = &depthAttachment;

        m_renderPass = m_device->createRenderPass(renderPassDesc);
    }
}
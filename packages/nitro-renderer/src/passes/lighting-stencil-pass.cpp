#include <nitro-renderer/passes/lighting-stencil-pass.h>
#include <nitro-geometry/vertex.h>
#include <nitro-renderer/render-object.h>

namespace nitro::renderer
{
    LightingStencilPass::LightingStencilPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, GBuffer &gBuffer,
                                             std::string shaderDir,
                                             bool isMetal) : m_device(device),
                                                             m_frameWidth(width),
                                                             m_frameHeight(height)
    {

        rhi::TextureDesc textureDesc;
        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorSRGBA16;
        textureDesc.size = {m_frameWidth, m_frameHeight};
        textureDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        m_lightingTexture = m_device->createTexture(textureDesc);
        std::vector<rhi::RHIDescriptorBinding> binding{
            {rhi::RHIDescriptorBinding::Type::UniformBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Both,
             2},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             3},
            {rhi::RHIDescriptorBinding::Type::Sampler,
             rhi::RHIDescriptorBinding::ShaderStage::Fragment,
             4},

        };

        m_descriptorLayout = m_device->createDescriptorLayout(binding);
        std::string shaderPath = shaderDir + "/lighting-stencil/lighting-stencil";
        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.cullMode = rhi::PipelineDesc::CullMode::Front;
        pipelineDesc.depthWrite = false;
        pipelineDesc.depthTest = CompareOp::Always;
        pipelineDesc.hasColorAttachment = false;
        pipelineDesc.hasPushConstant = true;
        pipelineDesc.hasDepth = false;
        pipelineDesc.pushConstantSize = sizeof(LightStencilPushConstant);
        pipelineDesc.layouts = {m_descriptorLayout};
        pipelineDesc.vertexLayout = geometry::Vertex::getVertexLayout();
        // RHIStencilDesc::StencilFace stencilFaceDesc;
        // stencilFaceDesc.depthFailOp = RHIStencilDesc::StencilOp::INCREMENT;

        // pipelineDesc.stencil.enabled = true;
        // pipelineDesc.stencil.front = stencilFaceDesc;
        // pipelineDesc.stencil.back = stencilFaceDesc;
        if (isMetal)
        {
            pipelineDesc.shaders.push_back({"vs", shaderPath + ".metallib", rhi::ShaderStage::Vertex});
        }
        else
        {
            pipelineDesc.shaders.push_back({"main", shaderPath + ".vert.spv", rhi::ShaderStage::Vertex});
        }

        // m_stencilPipeline = m_device->createPipeline(pipelineDesc);

        if (isMetal)
        {
            pipelineDesc.shaders.push_back({"fs", shaderPath + ".metallib", rhi::ShaderStage::Fragment});
        }
        else
        {
            pipelineDesc.shaders.push_back({"main", shaderPath + ".frag.spv", rhi::ShaderStage::Fragment});
        }

        rhi::RHIBlendDesc blendDesc;
        blendDesc.enabled = true;

        pipelineDesc.colorAttachments = {rhi::PipelineDesc::ColorAttachmentDesc(rhi::TextureDesc::ImageFormat::ColorSRGBA16, blendDesc)};
        pipelineDesc.hasColorAttachment = true;
        // stencilFaceDesc.compareOp = CompareOp::NotEqual;
        // stencilFaceDesc.depthFailOp = RHIStencilDesc::StencilOp::ZERO;
        // stencilFaceDesc.failOp = RHIStencilDesc::StencilOp::ZERO;
        // stencilFaceDesc.passOp = RHIStencilDesc::StencilOp::ZERO;
        // pipelineDesc.stencil.front = stencilFaceDesc;
        // pipelineDesc.stencil.back = stencilFaceDesc;
        // pipelineDesc.cullMode = PipelineDesc::CullMode::Front;

        m_lightVolumePipeline = m_device->createPipeline(pipelineDesc);

        rhi::RenderPassDesc renderPassDesc;

        renderPassDesc.width = m_frameWidth;
        renderPassDesc.height = m_frameHeight;

        rhi::RenderPassDesc::Attachment colorAttachment;
        colorAttachment.texture = m_lightingTexture;
        colorAttachment.depthWrite = false;
        colorAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        colorAttachment.store = rhi::RenderPassDesc::StoreOp::Store;

        renderPassDesc.colorAttachments = {colorAttachment};

        m_lightVolumePass = m_device->createRenderPass(renderPassDesc);

        m_resources.create(
            g_MAX_FRAMES_IN_FLIGHT,
            [&, gBuffer](uint32_t frame)
            {
                LightingStencilResource resource;

                rhi::BufferDesc uboDesc;
                uboDesc.size = sizeof(LightStencilCamera);
                uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                uboDesc.usage = rhi::BufferDesc::Usage::Uniform;

                resource.uniformBuffer = m_device->createBuffer(uboDesc);
                resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);

                resource.descriptorSet->writeBuffer(resource.uniformBuffer, 2);
                resource.descriptorSet->writeTexture(gBuffer.depth, 3);
                resource.descriptorSet->writeTexture(gBuffer.normal, 4);

                resource.descriptorSet->commit();
                return resource;
            });
    }

    LightingStencilPass::~LightingStencilPass()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyBuffer(resource.uniformBuffer);
        }
        m_device->destroyTexture(m_lightingTexture);

        if (m_stencilPipeline)
        {
            m_device->destroyPipeline(m_stencilPipeline);
        }
        if (m_lightVolumePipeline)
        {
            m_device->destroyPipeline(m_lightVolumePipeline);
        }
    }

    void LightingStencilPass::resize(uint32_t width, uint32_t height, GBuffer &gBuffer)
    {

        m_frameWidth = width;
        m_frameHeight = height;

        m_device->destroyTexture(m_lightingTexture);
        rhi::TextureDesc textureDesc;
        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorSRGBA16;
        textureDesc.size = {m_frameWidth, m_frameHeight};
        textureDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;

        m_lightingTexture = m_device->createTexture(textureDesc);

        rhi::RenderPassDesc renderPassDesc;

        renderPassDesc.width = m_frameWidth;
        renderPassDesc.height = m_frameHeight;

        rhi::RenderPassDesc::Attachment colorAttachment;
        colorAttachment.texture = m_lightingTexture;
        colorAttachment.depthWrite = false;
        colorAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        colorAttachment.store = rhi::RenderPassDesc::StoreOp::Store;

        renderPassDesc.colorAttachments = {colorAttachment};

        m_lightVolumePass = m_device->createRenderPass(renderPassDesc);

        for (auto &resource : m_resources)
        {
            resource.descriptorSet->writeBuffer(resource.uniformBuffer, 2);
            resource.descriptorSet->writeTexture(gBuffer.depth, 3);
            resource.descriptorSet->writeTexture(gBuffer.normal, 4);

            resource.descriptorSet->commit();
        }
    }

    void LightingStencilPass::execute(rhi::RHICommandBuffer *cmd, LightingSettings &settings, LightStencilCamera camera)
    {
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());
        cmd->beginRenderPass(m_lightVolumePass);
        cmd->bindPipeline(m_lightVolumePipeline);
        resource.uniformBuffer->upload(&camera, sizeof(LightStencilCamera));

        cmd->bindDescriptorSet(resource.descriptorSet, 0);
        rhi::RHIViewport viewport;
        viewport.width = m_frameWidth;
        viewport.height = m_frameHeight;
        cmd->setViewPort(viewport);
        rhi::RHIScissor scissor;
        scissor.width = m_frameWidth;
        scissor.height = m_frameHeight;
        cmd->setScissor(scissor);

        for (auto &pointLight : settings.pointLights)
        {
            geometry::MeshTransformation transformation;
            transformation.translate(glm::vec3(pointLight.position.x, pointLight.position.y, pointLight.position.z));
            transformation.scale(glm::vec3(pointLight.radius + 1));

            LightStencilPushConstant pushConstant;
            pushConstant.intensity = pointLight.intensity;
            pushConstant.lightColor = pointLight.color;
            pushConstant.lightPosition = pointLight.position;
            pushConstant.radius = pointLight.radius;
            pushConstant.screenSize = glm::vec2(m_frameWidth, m_frameHeight);
            pushConstant.model = transformation.getTransform().model;

            cmd->setPushConstant(&pushConstant, sizeof(LightStencilPushConstant), 1);

            settings.pointLightRenderer->draw(cmd);
        }

        cmd->endRenderPass();
    }
} // namespace nitro::renderer

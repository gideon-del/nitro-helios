#include <nitro-renderer/passes/tone-map-pass.h>

namespace nitro::renderer
{

    ToneMapPass::ToneMapPass(
        std::shared_ptr<rhi::RHIDevice> device,
        uint32_t width,
        uint32_t height,
        std::string shaderDir,
        bool isMetal)
        : m_device(device),
          m_width(width),
          m_height(height)
    {

        std::vector<rhi::RHIDescriptorBinding> bindings{
            {rhi::RHIDescriptorBinding::Type::Sampler, rhi::RHIDescriptorBinding::ShaderStage::Fragment, 2}};

        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        std::string shaderPath = shaderDir + "/tone-map-pass/tone-map-pass";
        rhi::PipelineDesc pipelineDesc;
        pipelineDesc.depthTest = rhi::CompareOp::Always;
        pipelineDesc.depthWrite = false;
        pipelineDesc.hasDepth = false;
        pipelineDesc.hasStencil = false;
        pipelineDesc.hasPushConstant = true;
        pipelineDesc.pushConstantSize = sizeof(ToneMapPassUBO);
        pipelineDesc.cullMode = rhi::PipelineDesc::CullMode::None;
        pipelineDesc.layouts = {m_descriptorLayout};
        pipelineDesc.colorAttachments = {rhi::TextureDesc::ImageFormat::ColorRGBA8};
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

        rhi::TextureDesc textureDesc;

        textureDesc.size = {m_width, m_height};
        textureDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;
        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;

        m_toneMappedTexture = m_device->createTexture(textureDesc);

        rhi::RenderPassDesc renderPassDesc;

        rhi::RenderPassDesc::Attachment colorAttachment;
        colorAttachment.texture = m_toneMappedTexture;
        colorAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        colorAttachment.store = rhi::RenderPassDesc::StoreOp::Store;

        renderPassDesc.colorAttachments = {colorAttachment};
        renderPassDesc.height = m_height;
        renderPassDesc.width = m_width;

        m_renderPass = m_device->createRenderPass(renderPassDesc);

        m_descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
    };

    ToneMapPass::~ToneMapPass()
    {
        m_device->destroyDescriptorSet(m_descriptorSet);
        m_device->destroyPipeline(m_pipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
        m_device->destroyTexture(m_toneMappedTexture);
        m_device->destroyRenderPass(m_renderPass);
    }

    void ToneMapPass::resize(
        uint32_t width,
        uint32_t height)
    {
        m_device->destroyTexture(m_toneMappedTexture);
        m_device->destroyRenderPass(m_renderPass);
        m_width = width;
        m_height = height;
        rhi::TextureDesc textureDesc;

        textureDesc.size = {m_width, m_height};
        textureDesc.usage = rhi::TextureDesc::Usage::RenderTarget | rhi::TextureDesc::Usage::ShaderRead;
        textureDesc.format = rhi::TextureDesc::ImageFormat::ColorRGBA8;

        m_toneMappedTexture = m_device->createTexture(textureDesc);

        rhi::RenderPassDesc renderPassDesc;

        rhi::RenderPassDesc::Attachment colorAttachment;
        colorAttachment.texture = m_toneMappedTexture;
        colorAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        colorAttachment.store = rhi::RenderPassDesc::StoreOp::Store;

        renderPassDesc.colorAttachments = {colorAttachment};
        renderPassDesc.height = m_height;
        renderPassDesc.width = m_width;

        m_renderPass = m_device->createRenderPass(renderPassDesc);
    }

    void ToneMapPass::execute(rhi::RHICommandBuffer *cmd, ToneMapPassUBO ubo, rhi::RHITexture *hdrTexture)
    {
        cmd->beginRenderPass(m_renderPass);
        cmd->bindPipeline(m_pipeline);
        rhi::TextureBinding textureBinding;
        textureBinding.texture = hdrTexture;
        textureBinding.sampler = m_device->defaultSamplers().linearRepeat;
        m_descriptorSet->writeTexture(textureBinding, 2, rhi::ImageLayout::ShaderReadOnly);
        m_descriptorSet->commit();
        cmd->bindDescriptorSet(m_descriptorSet, 0);
        cmd->setPushConstant(&ubo, sizeof(ToneMapPassUBO), 1);
        rhi::RHIViewport viewport;
        viewport.width = m_width;
        viewport.height = m_height;
        cmd->setViewPort(viewport);
        rhi::RHIScissor scissor;
        scissor.width = m_width;
        scissor.height = m_height;
        cmd->setScissor(scissor);
        cmd->draw(3);
        cmd->endRenderPass();
    }

} // namespace nitro::renderer

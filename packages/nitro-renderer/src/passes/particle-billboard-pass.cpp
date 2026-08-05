#include <nitro-renderer/passes/particle-billboard-pass.h>

namespace nitro::renderer
{
    ParticleBillboardPass::ParticleBillboardPass(std::shared_ptr<rhi::RHIDevice> device, uint32_t width, uint32_t height, std::string shaderDir, bool isMetal)
        : m_device(device),
          m_width(width),
          m_height(height)
    {

        std::vector<rhi::RHIDescriptorBinding> bindings{
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Vertex,
             2},
            {rhi::RHIDescriptorBinding::Type::UniformBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Vertex,
             3},
            {rhi::RHIDescriptorBinding::Type::StorageBuffer,
             rhi::RHIDescriptorBinding::ShaderStage::Vertex,
             4},

        };

        m_descriptorLayout = m_device->createDescriptorLayout(bindings);

        rhi::PipelineDesc pipelineDesc;

        pipelineDesc.layouts = {m_descriptorLayout};

        rhi::PipelineDesc::ColorAttachmentDesc colorAttachment{rhi::TextureDesc::ImageFormat::ColorRGBA16};
        colorAttachment.blend.enabled = false;
        pipelineDesc.colorAttachments = {colorAttachment};
        pipelineDesc.cullMode = rhi::PipelineDesc::CullMode::None;
        pipelineDesc.depthWrite = false;
        pipelineDesc.hasDepth = false;
        pipelineDesc.hasPushConstant = false;

        std::string shaderPath = shaderDir + "/particle-billboard/particle-billboard";

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
            [&](uint32_t frameIdx)
            {
                ParticleBillBoardResource resource;

                rhi::BufferDesc uboDesc;
                uboDesc.usage = rhi::BufferDesc::Usage::Uniform;
                uboDesc.storage = rhi::BufferDesc::StorageMode::Shared;
                uboDesc.size = sizeof(ParticleBillboardUBO);

                resource.uniformBuffer = m_device->createBuffer(uboDesc);

                resource.descriptorSet = m_device->createDescriptorSet(m_descriptorLayout);
                return resource;
            });
    }

    void ParticleBillboardPass::bindResources(const RGResources &resources, const ParticleBillboardResourceIDs particleResource)
    {
        if (m_renderPass)
        {
            m_device->destroyRenderPass(m_renderPass);
        }

        rhi::RenderPassDesc renderPassDesc;

        rhi::RenderPassDesc::Attachment colorAttachment;

        colorAttachment.texture = resources.getTexture(particleResource.outputTexture);
        colorAttachment.load = rhi::RenderPassDesc::LoadOp::Clear;
        colorAttachment.store = rhi::RenderPassDesc::StoreOp::Store;

        renderPassDesc.colorAttachments = {colorAttachment};
        renderPassDesc.width = m_width;
        renderPassDesc.height = m_height;

        m_renderPass = m_device->createRenderPass(renderPassDesc);

        for (auto &resource : m_resources)
        {
            resource.descriptorSet->writeBuffer(resources.getBuffer(particleResource.particleId), 2);
            resource.descriptorSet->writeBuffer(resource.uniformBuffer, 3);
            resource.descriptorSet->writeBuffer(resources.getBuffer(particleResource.aliveList), 4);
            resource.descriptorSet->commit();
        }
    };

    ParticleBillboardPass::~ParticleBillboardPass()
    {
        for (auto &resource : m_resources)
        {
            m_device->destroyBuffer(resource.uniformBuffer);
            m_device->destroyDescriptorSet(resource.descriptorSet);
        }

        m_device->destroyRenderPass(m_renderPass);
        m_device->destroyPipeline(m_pipeline);
        m_device->destroyDescriptorLayout(m_descriptorLayout);
    }

    void ParticleBillboardPass::resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    };

    void ParticleBillboardPass::execute(rhi::RHICommandBuffer *cmd, const ParticleBillboardUBO ubo, rhi::RHIBuffer *indirectDraw)
    {
        auto &resource = m_resources.current(m_device->getCurrentFrameIndex());

        resource.uniformBuffer->upload(&ubo, sizeof(ParticleBillboardUBO));

        cmd->beginRenderPass(m_renderPass);
        cmd->bindPipeline(m_pipeline);
        cmd->bindDescriptorSet(resource.descriptorSet, 0);

        rhi::RHIViewport viewport;
        viewport.width = m_width;
        viewport.height = m_height;

        cmd->setViewPort(viewport);

        rhi::RHIScissor scissors;

        scissors.width = m_width;
        scissors.height = m_height;

        cmd->setScissor(scissors);

        cmd->drawIndirect(indirectDraw);

        cmd->endRenderPass();
    }
} // namespace nitro::renderer

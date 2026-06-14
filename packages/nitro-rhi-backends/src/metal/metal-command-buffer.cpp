#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-swapchain.h>
#include <nitro-rhi-backends/metal/metal-command-buffer.h>
#include <nitro-rhi-backends/metal/metal-pipeline.h>
#include <nitro-rhi-backends/metal/metal-buffer.h>
#include <nitro-rhi-backends/metal/metal-texture.h>
#include <nitro-rhi-backends/metal/metal-descriptor-set.h>
#include <nitro-rhi-backends/metal/metal-render-pass.h>
#include <nitro-rhi-backends/metal/metal-descriptor-layout.h>

namespace nitro::rhi::metal
{
    MetalCommandBuffer::MetalCommandBuffer(MetalDevice *device, MetalSwapchain *swapchain) : m_device(device), swapchain(swapchain)
    {
        commandBuffer = m_device->commandQueue->commandBuffer();
    }
    void MetalCommandBuffer::beginRenderPass(const RHIRenderPassDesc &desc)
    {
        rpd = MTL::RenderPassDescriptor::alloc()->init();

        rpd->colorAttachments()->object(0)->setTexture(
            swapchain->currentDrawable->texture());
        rpd->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
        rpd->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
        rpd->colorAttachments()->object(0)->setClearColor(
            MTL::ClearColor(desc.clearColor[0], desc.clearColor[1],
                            desc.clearColor[2], desc.clearColor[3]));

        rpd->depthAttachment()->setTexture(swapchain->depthTexture->texture);
        rpd->depthAttachment()->setLoadAction(MTL::LoadActionClear);
        rpd->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
        rpd->depthAttachment()->setClearDepth(desc.clearDepth);

        encoder = commandBuffer->renderCommandEncoder(rpd);
        encoder->setCullMode(MTL::CullModeBack);
        encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
        // encoder->setDepthBias(2.0, 2.0, 0.0);
    }
    void MetalCommandBuffer::beginRenderPass(RHIRenderPass *renderPass)
    {
        MetalRenderPass *metalRenderPass = reinterpret_cast<MetalRenderPass *>(renderPass);

        encoder = commandBuffer->renderCommandEncoder(metalRenderPass->renderPassDescriptor);
        encoder->setCullMode(MTL::CullModeBack);
        encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
        encoder->setDepthBias(metalRenderPass->depthBiasConstant, metalRenderPass->depthBiasSlopScale, metalRenderPass->depthBiasSlopScale);
    }
    void MetalCommandBuffer::endRenderPass()
    {
        encoder->endEncoding();
    }

    void MetalCommandBuffer::setViewPort(const RHIViewport &viewport)
    {
        MTL::Viewport metalViewport;
        metalViewport.originX = viewport.x;
        metalViewport.originY = viewport.y;
        metalViewport.width = viewport.width;
        metalViewport.height = viewport.height;
        metalViewport.zfar = viewport.maxDepth;
        metalViewport.znear = viewport.minDepth;

        encoder->setViewport(metalViewport);
    }

    void MetalCommandBuffer::setScissor(const RHIScissor &scissor)
    {
        MTL::ScissorRect metalScissor;
        metalScissor.x = scissor.x;
        metalScissor.y = scissor.y;
        metalScissor.width = scissor.width;
        metalScissor.height = scissor.height;
        encoder->setScissorRect(metalScissor);
    }
    void MetalCommandBuffer::bindPipeline(RHIPipeline *pipeline)
    {
        MetalPipeline *metalPipeline = reinterpret_cast<MetalPipeline *>(pipeline);

        encoder->setRenderPipelineState(metalPipeline->pipelineState);
        encoder->setDepthStencilState(
            metalPipeline->depthStencilState);
        m_pipeline = metalPipeline;
    }

    void MetalCommandBuffer::bindVertexBuffer(RHIBuffer *buffer)
    {
        MetalBuffer *metalBuffer = reinterpret_cast<MetalBuffer *>(buffer);
        encoder->setVertexBuffer(metalBuffer->buffer, NS::UInteger(0), NS::UInteger(0));
    }

    void MetalCommandBuffer::bindIndexBuffer(RHIBuffer *buffer)
    {
        MetalBuffer *metalBuffer = reinterpret_cast<MetalBuffer *>(buffer);
        m_currentIndexBuffer = metalBuffer;
    }

    void MetalCommandBuffer::bindUniformBuffer(RHIBuffer *buffer, uint32_t binding)
    {
        MetalBuffer *metalBuffer = reinterpret_cast<MetalBuffer *>(buffer);
        encoder->setVertexBuffer(metalBuffer->buffer, NS::UInteger(0), NS::UInteger(binding));
    }

    void MetalCommandBuffer::setPushConstant(void *data, size_t size, uint32_t binding)
    {
        encoder->setVertexBytes(data, NS::UInteger(size), NS::UInteger(binding));
    }
    void MetalCommandBuffer::draw(uint32_t vertexCount)
    {

        if (!m_pipeline)
        {
            throw std::runtime_error("Must bind pipeline for draws");
        }
        m_FrameStats.drawCalls += 1;
        m_FrameStats.triangles += vertexCount / 3;
        encoder->drawPrimitives(m_pipeline->topology, NS::UInteger(0), NS::UInteger(vertexCount));
    }
    void MetalCommandBuffer::bindDescriptorSet(RHIDescriptorSet *set, uint32_t mainBinding)
    {
        MetalDescriptorSet *metalSet = reinterpret_cast<MetalDescriptorSet *>(set);

        for (auto &[buffer, binding] : metalSet->bufferBindings)
        {
            MetalBuffer *metalBuffer = reinterpret_cast<MetalBuffer *>(buffer);
            if (metalSet->descriptorLayout->bufferBindings[binding] == RHIDescriptorBinding::ShaderStage::Vertex || metalSet->descriptorLayout->bufferBindings[binding] == RHIDescriptorBinding::ShaderStage::Both)
            {
                encoder->setVertexBuffer(metalBuffer->buffer, 0, MetalDescriptorSet::s_getMetalBufferBinding(mainBinding, binding));
            }
            if (metalSet->descriptorLayout->bufferBindings[binding] == RHIDescriptorBinding::ShaderStage::Fragment || metalSet->descriptorLayout->bufferBindings[binding] == RHIDescriptorBinding::ShaderStage::Both)
            {
                encoder->setFragmentBuffer(metalBuffer->buffer, 0, MetalDescriptorSet::s_getMetalTextureBinding(mainBinding, binding));
            }
        }
        for (auto &[texture, binding] : metalSet->textureBindings)
        {
            MetalTexture *metalTex = reinterpret_cast<MetalTexture *>(texture);
            encoder->setFragmentTexture(metalTex->texture, MetalDescriptorSet::s_getMetalTextureBinding(mainBinding, binding));
            encoder->setFragmentSamplerState(metalTex->samplerState, mainBinding);
        }
    }
    void MetalCommandBuffer::drawIndexed(uint32_t indexCount)
    {
        if (!m_pipeline)
        {
            throw std::runtime_error("Must bind pipeline for draws");
        }

        m_FrameStats.drawCalls += 1;
        m_FrameStats.triangles += indexCount / 3;

        encoder->drawIndexedPrimitives(m_pipeline->topology,
                                       NS::UInteger(indexCount),
                                       MTL::IndexTypeUInt32,
                                       m_currentIndexBuffer->buffer,
                                       NS::UInteger(0));
    }
    void MetalCommandBuffer::present()
    {
        commandBuffer->presentDrawable(swapchain->currentDrawable);
    }
    MetalCommandBuffer::~MetalCommandBuffer()
    {
        if (encoder)
            encoder->release();
        if (commandBuffer)
            commandBuffer->release();
    }

    FrameStats MetalCommandBuffer::getFrameStats()
    {
        return m_FrameStats;
    }
    void MetalCommandBuffer::resetFrameStats()
    {
        m_FrameStats.drawCalls = 0;
        m_FrameStats.triangles = 0;
        m_FrameStats.vertices = 0;
    }
    void MetalCommandBuffer::updateVertexCount(uint32_t count)
    {
        m_FrameStats.vertices += count;
    }
} // namespace nitro::rhi::metal

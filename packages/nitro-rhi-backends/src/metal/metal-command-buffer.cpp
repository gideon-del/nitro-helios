#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-swapchain.h>
#include <nitro-rhi-backends/metal/metal-command-buffer.h>
#include <nitro-rhi-backends/metal/metal-pipeline.h>
#include <nitro-rhi-backends/metal/metal-buffer.h>
#include <nitro-rhi-backends/metal/metal-texture.h>
#include <nitro-rhi-backends/common/push-constant.h>

namespace nitro::rhi::metal
{
    MetalCommandBuffer::MetalCommandBuffer(MetalDevice *device, MetalSwapchain *swapchain) : m_device(device), swapchain(swapchain)
    {
        BufferDesc bufferDesc;
        bufferDesc.storage = BufferDesc::StorageMode::Shared;
        bufferDesc.usage = BufferDesc::Usage::Uniform;
        bufferDesc.size = sizeof(PushConstant);

        commandBuffer = m_device->commandQueue->commandBuffer();
    }
    void MetalCommandBuffer::beginRenderPass(const RHIRenderPassDesc &desc)
    {
        MTL::RenderPassDescriptor *rpd = MTL::RenderPassDescriptor::alloc()->init();

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
        encoder->setCullMode(MTL::CullModeNone);
        encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
        rpd->release();
    }

    void MetalCommandBuffer::endRenderPass()
    {
        encoder->endEncoding();
    }

    void MetalCommandBuffer::bindPipeline(RHIPipeline *pipeline)
    {
        MetalPipeline *metalPipeline = reinterpret_cast<MetalPipeline *>(pipeline);

        encoder->setRenderPipelineState(metalPipeline->pipelineState);
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
        encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(vertexCount));
    }
    void MetalCommandBuffer::drawIndexed(uint32_t indexCount)
    {
        if (!m_currentIndexBuffer)
        {
            throw std::runtime_error("Must bind index buffer");
        }

        encoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
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
} // namespace nitro::rhi::metal

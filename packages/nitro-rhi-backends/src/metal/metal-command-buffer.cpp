#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-swapchain.h>
#include <nitro-rhi-backends/metal/metal-command-buffer.h>
#include <nitro-rhi-backends/metal/metal-pipeline.h>

namespace nitro::rhi::metal
{
    MetalCommandBuffer::MetalCommandBuffer(MetalDevice *device, MetalSwapchain *swapchain) : m_device(device), swapchain(swapchain)
    {
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

        rpd->depthAttachment()->setTexture(swapchain->depthTexture);
        rpd->depthAttachment()->setLoadAction(MTL::LoadActionClear);
        rpd->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
        rpd->depthAttachment()->setClearDepth(desc.clearDepth);

        encoder = commandBuffer->renderCommandEncoder(rpd);

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
        // TODO
    }

    void MetalCommandBuffer::bindIndexBuffer(RHIBuffer *buffer)
    {
        // TODO
    }

    void MetalCommandBuffer::bindUniformBuffer(RHIBuffer *buffer, uint32_t binding)
    {
        // TODO
    }
    void MetalCommandBuffer::drawIndexed(uint32_t indexCount)
    {
        encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(indexCount));
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

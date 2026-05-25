#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-command-buffer.h>
#include <nitro-rhi-backends/metal/metal-swapchain.h>
#include <nitro-rhi-backends/metal/metal-pipeline.h>
#include <nitro-rhi-backends/metal/metal-buffer.h>
#include <nitro-rhi-backends/metal/metal-texture.h>

namespace nitro::rhi::metal
{
    MetalDevice::MetalDevice(void *window) : m_window(window)
    {
        device = MTL::CreateSystemDefaultDevice();
        commandQueue = device->newCommandQueue();
    }
    MetalDevice::~MetalDevice()
    {
        commandQueue->release();
        device->release();
    }

    RHIBuffer *MetalDevice::createBuffer(const BufferDesc &desc)
    {
        return new MetalBuffer(this, desc);
    }
    RHITexture *MetalDevice::createTexture(const TextureDesc &desc)
    {
        return new MetalTexture(this, desc);
    }
    RHIPipeline *MetalDevice::createPipeline(const PipelineDesc &desc)
    {
        return new MetalPipeline(this, desc);
    }
    void MetalDevice::destroyBuffer(RHIBuffer *buffer)
    {
        delete buffer;
    }

    void MetalDevice::destroyTexture(RHITexture *texture)
    {
        delete texture;
    }
    void MetalDevice::destroyPipeline(RHIPipeline *pipeline)
    {
        delete pipeline;
    }

    RHISwapchain *MetalDevice::createSwapchain(RHISurface *surface)
    {
        MetalSwapchain *swapchain = new MetalSwapchain(this, m_window);

        m_swapchain = swapchain;

        return swapchain;
    }
    RHICommandBuffer *MetalDevice::beginFrame()
    {
        if (!m_swapchain)
        {
            throw std::runtime_error("Metal Swapchain not found");
        }
        m_swapchain->currentDrawable = m_swapchain->layer->nextDrawable();
        return new MetalCommandBuffer(this, m_swapchain);
    }
    void MetalDevice::endFrame(RHICommandBuffer *cmd)
    {
        MetalCommandBuffer *metalCmd = reinterpret_cast<MetalCommandBuffer *>(cmd);

        metalCmd->commandBuffer->commit();
        delete cmd;
    }
    void MetalDevice::waitIdle() {};

} // namespace nitro::rhi::metal

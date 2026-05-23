#include <nitro-rhi-backends/metal/metal-device.h>

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
        return nullptr;
    }
    RHITexture *MetalDevice::createTexture(const TextureDesc &desc)
    {
        return nullptr;
    }
    RHIPipeline *MetalDevice::createPipeline(const PipelineDesc &desc)
    {
        return nullptr;
    }
    void MetalDevice::destroyBuffer(RHIBuffer *buffer) {}

    void MetalDevice::destroyTexture(RHITexture *texture) {}
    void MetalDevice::destroyPipeline(RHIPipeline *pipeline) {}

    RHICommandBuffer *MetalDevice::beginFrame()
    {
        return nullptr;
    }
    void MetalDevice::endFrame(RHICommandBuffer *cmd)
    {
    }

} // namespace nitro::rhi::metal

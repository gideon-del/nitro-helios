#include <nitro-rhi-backends/metal/metal-device.h>
#include <nitro-rhi-backends/metal/metal-command-buffer.h>
#include <nitro-rhi-backends/metal/metal-swapchain.h>
#include <nitro-rhi-backends/metal/metal-pipeline.h>
#include <nitro-rhi-backends/metal/metal-buffer.h>
#include <nitro-rhi-backends/metal/metal-texture.h>
#include <nitro-rhi-backends/metal/metal-descriptor-layout.h>
#include <nitro-rhi-backends/metal/metal-descriptor-set.h>

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
    RHIDescriptorLayout *MetalDevice::createDescriptorLayout(const std::vector<RHIDescriptorBinding> bindings)
    {
        return new MetalDescriptorLayout(this, bindings);
    }
    RHIDescriptorSet *MetalDevice::createDescriptorSet(RHIDescriptorLayout *layout)
    {
        return new MetalDescriptorSet(this);
    }
    RHIRenderPass *MetalDevice::createRenderPass(const RenderPassDesc &desc)
    {
        return nullptr;
    };
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

    uint32_t MetalDevice::getCurrentFrameIndex() const
    {
        return 0;
    }
} // namespace nitro::rhi::metal

#pragma once
#include <nitro-rhi/rhi-device.h>
#include <SingleHeader/MetalCpp.h>
namespace nitro::rhi::metal
{

    class MetalSwapchain;
    class MetalDevice : public RHIDevice
    {
    public:
        MetalDevice(void *window);
        ~MetalDevice() override;
        RHISwapchain *createSwapchain(RHISurface *surface) override;

        RHIPipeline *createPipeline(const PipelineDesc &desc) override;
        void destroyPipeline(RHIPipeline *pipeline) override;

        RHIBuffer *createBuffer(const BufferDesc &desc) override;
        void destroyBuffer(RHIBuffer *buffer) override;

        RHITexture *createTexture(const TextureDesc &desc) override;
        void destroyTexture(RHITexture *texture) override;

        RHICommandBuffer *beginFrame() override;
        void endFrame(RHICommandBuffer *cmd) override;

        void waitIdle();
        MTL::Device *device;
        MTL::CommandQueue *commandQueue;

    private:
        MetalSwapchain *m_swapchain = nullptr;
        void *m_window = nullptr;
    };
} // namespace nitro::rhi::metal

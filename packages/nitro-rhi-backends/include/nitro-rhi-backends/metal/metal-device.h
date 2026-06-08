#pragma once
#include <nitro-rhi/rhi-device.h>
#include <Metal/Metal.hpp>
namespace nitro::rhi::metal
{

    class MetalSwapchain;
    class MetalCommandBuffer;
    class MetalDevice : public RHIDevice
    {
    public:
        MetalDevice(void *window);
        ~MetalDevice() override;
        RHISwapchain *createSwapchain(RHISurface *surface) override;

        RHIDescriptorLayout *createDescriptorLayout(const std::vector<RHIDescriptorBinding> bindings) override;
        RHIDescriptorSet *createDescriptorSet(RHIDescriptorLayout *layout) override;
        RHITimer *createTimer() override;
        RHIRenderPass *createRenderPass(const RenderPassDesc &desc) override;

        RHIPipeline *createPipeline(const PipelineDesc &desc) override;
        void destroyPipeline(RHIPipeline *pipeline) override;

        RHIBuffer *createBuffer(const BufferDesc &desc) override;
        void destroyBuffer(RHIBuffer *buffer) override;

        RHITexture *createTexture(const TextureDesc &desc) override;
        void destroyTexture(RHITexture *texture) override;

        RHICommandBuffer *beginFrame() override;
        void endFrame(RHICommandBuffer *cmd) override;
        uint32_t getCurrentFrameIndex() const override;
        void beginImGuiFrame() override;
        void endImGuiFrame() override;
        void drawImGui(RHICommandBuffer *cmd) override;

        void waitIdle();
        MTL::Device *device;
        MTL::CommandQueue *commandQueue;

    private:
        MetalSwapchain *m_swapchain = nullptr;
        void *m_window = nullptr;
        MetalCommandBuffer *m_currentCommandBuffer = nullptr;
    };
} // namespace nitro::rhi::metal

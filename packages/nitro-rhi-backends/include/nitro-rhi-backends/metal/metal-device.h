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
        void destroyDescriptorLayout(RHIDescriptorLayout *layout) override;

        RHIDescriptorSet *createDescriptorSet(RHIDescriptorLayout *layout) override;
        void destroyDescriptorSet(RHIDescriptorSet *set) override;
        RHITimer *createTimer() override;

        RHIRenderPass *createRenderPass(const RenderPassDesc &desc) override;
        void destroyRenderPass(RHIRenderPass *renderPass) override;

        RHIPipeline *createPipeline(const PipelineDesc &desc) override;
        void destroyPipeline(RHIPipeline *pipeline) override;

        RHIBuffer *createBuffer(const BufferDesc &desc) override;
        void destroyBuffer(RHIBuffer *buffer) override;

        RHITexture *createTexture(const TextureDesc &desc) override;
        void destroyTexture(RHITexture *texture) override;

        RHIComputePipeline *createComputePipeline(const ComputePipelineDesc &desc) override;
        void destroyComputePipeline(RHIComputePipeline *pipeline) override;

        RHISamplerHandle create(const RHISamplerDesc &desc) override;
        void destroy(RHISamplerHandle &handle) override;

        const DefaultSamplers &defaultSamplers() const override { return m_defaultSamplers; };
        RHICommandBuffer *beginFrame() override;
        void endFrame(RHICommandBuffer *cmd) override;
        uint32_t getCurrentFrameIndex() const override;
        void beginImGuiFrame() override;
        void endImGuiFrame() override;
        void drawImGui(RHICommandBuffer *cmd) override;
        RHICommandBuffer *createCommandBuffer() override;
        void endCommandBuffer(RHICommandBuffer *cmd) override;
        void *getImGuiTextureRef(RHITexture *texture) override;
        void waitIdle() override;
        MTL::Device *device;
        MTL::CommandQueue *commandQueue;

        MemoryRequirements textureMemoryRequirements(const TextureDesc &desc) override;

        RHIHeap *createHeap(size_t sizeBytes, uint32_t memoryTypeBits) override;
        void destroyHeap(RHIHeap *heap) override;
        RHITexture *createTextureFromHeap(RHIHeap *heap, const TextureDesc &desc, size_t offset) override;

    private:
        MetalSwapchain *m_swapchain = nullptr;
        void *m_window = nullptr;
        MetalCommandBuffer *m_currentCommandBuffer = nullptr;
        DefaultSamplers m_defaultSamplers;
    };
} // namespace nitro::rhi::metal

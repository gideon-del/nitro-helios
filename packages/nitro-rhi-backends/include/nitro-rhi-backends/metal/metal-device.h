#pragma once
#include <nitro-rhi/rhi-device.h>
#include <Metal/Metal.hpp>
#include "nitro-core/types/pool.h"
#include "nitro-rhi/rhi-deffered-deletion-queue.h"
#include "nitro-rhi/rhi-constants.h"
namespace nitro::rhi::metal
{

    class MetalSwapchain;
    class MetalCommandBuffer;
    class MetalTexture;
    class MetalHeap;
    class MetalBuffer;

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

        RHIBufferHandle create(const BufferDesc &desc) override;
        RHIBuffer *resolve(const RHIBufferHandle &handle) override;
        void destroy(const RHIBufferHandle &handle) override;

        RHIBuffer *createBuffer(const BufferDesc &desc) override;
        void destroyBuffer(RHIBuffer *buffer) override;

        RHITextureHandle create(const TextureDesc &desc) override;
        RHITexture *resolve(const RHITextureHandle &handle) override;
        void destroy(const RHITextureHandle &handle) override;

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
        uint64_t getFrameCompleted() const
        {
            return m_frameEvent->signaledValue();
        }
        void waitForFrame()
        {
            if (m_frameNumber < kMaxFramesInFlight)
            {
                return;
            }
            auto maxFrame = m_frameNumber + 1 - kMaxFramesInFlight;
            if (getFrameCompleted() < maxFrame)
            {
                m_frameEvent->waitUntilSignaledValue(maxFrame, UINT64_MAX);
            }
        }
        void collectGarbage();
        uint64_t retireAt() const
        {
            return m_frameNumber + 1;
        }

    private:
        MetalSwapchain *m_swapchain = nullptr;
        void *m_window = nullptr;
        MTL::SharedEvent *m_frameEvent = nullptr;
        uint64_t m_frameNumber = 0;
        MetalCommandBuffer *m_currentCommandBuffer = nullptr;
        DefaultSamplers m_defaultSamplers;

        ResourcePool<std::unique_ptr<MetalTexture>, RHITextureTag> m_texturePool;
        ResourcePool<std::unique_ptr<MetalBuffer>, RHIBufferTag> m_bufferPool;
        ResourcePool<std::unique_ptr<MetalHeap>, RHIHeapTag> m_heapPool;

        DeferredDeletionQueue<std::unique_ptr<MetalTexture>> m_textureGraveyard;
        DeferredDeletionQueue<std::unique_ptr<MetalBuffer>> m_bufferGraveyard;
        DeferredDeletionQueue<std::unique_ptr<MetalHeap>> m_heapGraveyard;
    };
} // namespace nitro::rhi::metal

#pragma once
#include "nitro-rhi/rhi-device.h"

namespace nitro::tests
{
    using namespace nitro::rhi;
    class CountingDevice : public RHIDevice
    {

        uint32_t m_creates = 0;
        uint32_t m_live = 0;
        uint32_t m_destroys = 0;
        uint32_t m_next = 1;
        DefaultSamplers m_samplers{};
        uint32_t m_heapCreates = 0, m_heapLive = 0;

    public:
        CountingDevice() {}
        ~CountingDevice() override {}
        RHISwapchain *createSwapchain(RHISurface *surface) override { return nullptr; }
        RHIDescriptorLayout *createDescriptorLayout(const std::vector<RHIDescriptorBinding> bindings) override { return nullptr; }
        void destroyDescriptorLayout(RHIDescriptorLayout *layout) override {}
        RHIDescriptorSet *createDescriptorSet(RHIDescriptorLayout *layout) override { return nullptr; }
        void destroyDescriptorSet(RHIDescriptorSet *set) override {};

        RHITimer *createTimer() override { return nullptr; }

        RHIRenderPass *createRenderPass(const RenderPassDesc &desc) override { return nullptr; }

        void destroyRenderPass(RHIRenderPass *renderPass) override {}

        RHITexture *resolve(const RHITextureHandle &handle) override { return nullptr; }
        RHITextureHandle create(const TextureDesc &desc) override
        {
            m_creates++;
            m_live++;
            return {m_next++, 1};
        }
        void destroy(const RHITextureHandle &handle) override
        {
            m_live--;
            m_destroys++;
        }

        RHIBuffer *resolve(const RHIBufferHandle &handle) override { return nullptr; }
        void destroy(const RHIBufferHandle &handle) override
        {
            m_live--;
            m_destroys++;
        }
        RHIBuffer *createBuffer(const BufferDesc &desc) override
        {
            return nullptr;
        };
        void destroyBuffer(RHIBuffer *buffer) override {};

        RHITexture *createTexture(const TextureDesc &desc) override
        {
            m_creates++;
            m_live++;
            return nullptr;
        }
        void destroyTexture(RHITexture *texture) override
        {
            m_live--;
            m_destroys++;
        }

        RHIBufferHandle create(const BufferDesc &desc) override
        {
            m_creates++;
            m_live++;

            return {m_next++, 1};
        }

        RHIPipeline *createPipeline(const PipelineDesc &desc) override { return nullptr; }
        void destroyPipeline(RHIPipeline *pipeline) override {}

        RHIComputePipeline *createComputePipeline(const ComputePipelineDesc &desc) override { return nullptr; }
        void destroyComputePipeline(RHIComputePipeline *pipeline) override {}

        MemoryRequirements textureMemoryRequirements(const TextureDesc &desc) override
        {
            return {.size = size_t(desc.size.width) * desc.size.height * 4,
                    .alignment = 64 * 1024,
                    .memoryTypeBits = 0b011};
        }
        RHISamplerHandle create(const RHISamplerDesc &desc) override { return {}; }

        void destroy(RHISamplerHandle &handle) override {};
        const DefaultSamplers &defaultSamplers() const override { return m_samplers; }

        void waitIdle() override {};

        uint32_t getCurrentFrameIndex() const override { return 0; }
        RHICommandBuffer *beginFrame() override { return nullptr; }
        RHICommandBuffer *createCommandBuffer() override { return nullptr; }

        void *getImGuiTextureRef(RHITexture *texture) override
        {
            return nullptr;
        }

        void endFrame(RHICommandBuffer *cmd) override
        {
        }

        void endCommandBuffer(RHICommandBuffer *cmd) override {}

        void beginImGuiFrame() override {};
        void endImGuiFrame() override {};
        void drawImGui(RHICommandBuffer *cmd) override {};

        RHIHeapHandle create(size_t sizeBytes, uint32_t memoryTypeBits) override
        {
            ++m_heapCreates;
            ++m_heapLive;
            return {m_next++, 1};
        };

        RHIHeap *resolve(const RHIHeapHandle &handle) override { return nullptr; }
        void destroy(const RHIHeapHandle &handle) override
        {
            m_heapLive--;
        }

        RHIHeap *createHeap(size_t sizeBytes, uint32_t memoryTypeBits) override { return nullptr; }
        void destroyHeap(RHIHeap *heap) override
        {
            m_heapLive--;
        }

        RHITexture *createTextureFromHeap(RHIHeap *heap, const TextureDesc &desc, size_t offset) override { return nullptr; }

        RHITextureHandle createTextureFromHeap(const RHIHeapHandle &heapHandle, const TextureDesc &desc, size_t offset) override
        {
            ++m_creates;
            ++m_live;
            return {m_next++, 1};
        }

        uint32_t getCreateCount() const { return m_creates; }
        uint32_t getLiveCount() const { return m_live; }
        uint32_t getDestroyCount() const { return m_destroys; }
        uint32_t getHeapCreateCount() const { return m_heapCreates; }
        uint32_t getHeapLiveCount() const { return m_heapLive; }
    };
}
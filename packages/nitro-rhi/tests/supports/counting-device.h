#pragma once
#include "nitro-rhi/rhi-device.h"
#include "fake-texture.h"
#include "fake-buffer.h"
#include "nitro-core/types/pool.h"
#include <memory>
#include <vector>

namespace nitro::tests
{
    using namespace nitro::rhi;

    // Add overrides here if RHIDescriptorLayout gains pure virtuals.
    struct FakeDescriptorLayout : RHIDescriptorLayout
    {
        std::vector<RHIDescriptorBinding> bindings;
    };

    class CountingDevice : public RHIDevice
    {

        ResourcePool<std::unique_ptr<FakeTexture>, RHITextureTag> m_textures;
        ResourcePool<std::unique_ptr<FakeBuffer>, RHIBufferTag> m_buffers;
        uint32_t m_creates = 0;
        uint32_t m_live = 0;
        uint32_t m_destroys = 0;
        uint32_t m_next = 1;
        DefaultSamplers m_samplers{};
        uint32_t m_heapCreates = 0, m_heapLive = 0;

        std::vector<std::unique_ptr<FakeDescriptorLayout>> m_layouts;
        uint32_t m_layoutCreates = 0;
        uint32_t m_layoutDestroys = 0;

    public:
        CountingDevice() {}
        ~CountingDevice() override {}
        RHISwapchain *createSwapchain(RHISurface *surface) override { return nullptr; }

        RHIDescriptorLayout *createDescriptorLayout(const std::vector<RHIDescriptorBinding> bindings, std::source_location loc = std::source_location::current()) override
        {
            ++m_layoutCreates;
            auto layout = std::make_unique<FakeDescriptorLayout>();
            layout->bindings = bindings;
            m_layouts.push_back(std::move(layout));
            return m_layouts.back().get();
        }
        void destroyDescriptorLayout(RHIDescriptorLayout *layout) override
        {
            if (layout)
                ++m_layoutDestroys;
        }

        RHIDescriptorSet *createDescriptorSet(RHIDescriptorLayout *layout, std::source_location loc = std::source_location::current()) override { return nullptr; }
        void destroyDescriptorSet(RHIDescriptorSet *set) override {};

        RHITimer *createTimer() override { return nullptr; }

        RHIRenderPass *createRenderPass(const RenderPassDesc &desc) override { return nullptr; }

        void destroyRenderPass(RHIRenderPass *renderPass) override {}

        RHITexture *resolve(const RHITextureHandle &handle) override
        {
            auto texture = m_textures.get(handle);
            if (!texture)
            {
                return nullptr;
            }

            return texture->get();
        }
        RHITextureHandle create(const TextureDesc &desc) override
        {
            m_creates++;
            m_live++;

            auto handle = m_textures.emplace(std::make_unique<FakeTexture>(desc.mipmaps, desc.type == TextureDesc::Type::Cube ? 6 : 1));

            auto texture = m_textures.get(handle)->get();
            assignHandle(*texture, handle);
            return handle;
        }
        void destroy(const RHITextureHandle &handle) override
        {
            m_live--;
            m_destroys++;
            m_textures.free(handle);
        }

        RHIBuffer *resolve(const RHIBufferHandle &handle) override
        {
            auto buffer = m_buffers.get(handle);

            if (!buffer)
            {
                return nullptr;
            }

            return buffer->get();
        }
        void destroy(const RHIBufferHandle &handle) override
        {
            m_live--;
            m_destroys++;

            m_buffers.free(handle);
        }
        RHIBuffer *createBuffer(const BufferDesc &desc) override
        {
            return resolve(create(desc));
        };
        void destroyBuffer(RHIBuffer *buffer) override
        {
            if (!buffer)
                return;
            destroy(buffer->getHandle());
        };

        RHITexture *createTexture(const TextureDesc &desc) override
        {

            return resolve(create(desc));
        }
        void destroyTexture(RHITexture *texture) override
        {
            if (!texture)
            {
                return;
            }

            auto handle = texture->getHandle();

            destroy(handle);
        }

        RHIBufferHandle create(const BufferDesc &desc) override
        {
            m_creates++;
            m_live++;

            auto handle = m_buffers.emplace(std::make_unique<FakeBuffer>());

            auto buffer = m_buffers.get(handle)->get();
            assignHandle(*buffer, handle);
            return handle;
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

        void destroyTimer(RHITimer *timer) override {}
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

        RHITexture *createTextureFromHeap(RHIHeap *heap, const TextureDesc &desc, size_t offset) override { return resolve(create(desc)); }

        RHITextureHandle createTextureFromHeap(const RHIHeapHandle &heapHandle, const TextureDesc &desc, size_t offset) override
        {

            return create(desc);
        }

        uint32_t getCreateCount() const { return m_creates; }
        uint32_t getLiveCount() const { return m_live; }
        uint32_t getDestroyCount() const { return m_destroys; }
        uint32_t getHeapCreateCount() const { return m_heapCreates; }
        uint32_t getHeapLiveCount() const { return m_heapLive; }
        uint32_t layoutCreateCount() const { return m_layoutCreates; }
        uint32_t layoutDestroyCount() const { return m_layoutDestroys; }
    };
}
#pragma once
#include <nitro-rhi/rhi.h>
#include <cstdint>
#include <functional>
#include <nitro-renderer/context.h>
#include <nitro-renderer/settings.h>
#include "graph.h"
#include <variant>
#include "per-frame.h"
namespace nitro::renderer
{
    using RGResourceID = uint32_t;
    using RGTextureID = RGResourceID;
    using RGBufferID = RGResourceID;

    struct RGTextureDesc
    {
        std::string name;
        rhi::TextureDesc::ImageFormat format;
        uint32_t width = 0, height = 0;
        bool isStorage = false;
        bool transient = true;
    };
    struct RGBufferDesc
    {
        std::string name;
        uint32_t size;
        rhi::BufferDesc::Usage usage;
        bool transient = false;
    };

    enum class WriteMode
    {
        Producer,
        Extend
    };
    struct RGResourceAccess
    {
        RGResourceID id;
        rhi::ResourceState state;
        WriteMode writeMode = WriteMode::Producer;
    };

    struct RGValidationError
    {
        std::string passName;
        std::string textureName;
        std::string message;
    };

    struct RGLifetime
    {
        RGResourceID id;
        int createdAt = -1;
        int lastUsedAt = -1;
    };

    struct RGAllocatedBuffer
    {
        std::array<rhi::RHIBuffer *, g_MAX_FRAMES_IN_FLIGHT> slots{};
        bool transient = false;
    };
    struct RGResources
    {
        std::unordered_map<RGResourceID, rhi::RHITexture *> allocatedTextures;
        std::unordered_map<RGResourceID, RGAllocatedBuffer> allocatedBuffers;
        rhi::RHITexture *getTexture(RGResourceID id) const
        {
            if (!allocatedTextures.count(id))
                return nullptr;
            return allocatedTextures.at(id);
        }
        rhi::RHIBuffer *getBuffer(RGResourceID id, uint32_t frameIdx = 0) const
        {
            if (!allocatedBuffers.count(id))
                return nullptr;
            auto &allocated = allocatedBuffers.at(id);
            return allocated.transient ? allocated.slots[frameIdx % g_MAX_FRAMES_IN_FLIGHT] : allocated.slots[0];
        }
    };

    struct RenderPass
    {
        std::string name;

        std::vector<RGResourceAccess> reads, writes;
        std::vector<RGResourceAccess> readBufs, writeBufs;
        std::function<void(const RGResources &resources)> bind;
        std::function<void(rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)> execute;
    };

    template <class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };

    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

    struct RGTextureBarrier
    {
        RGTextureID textureId;
        rhi::ResourceState from;
        rhi::ResourceState to;
    };
    struct RGBufferBarrier
    {
        RGBufferID bufferId;
        rhi::ResourceState from;
        rhi::ResourceState to;
    };
    struct RGCompiledFrameGraph
    {
        using StepVariant = std::variant<int, RGTextureBarrier, RGBufferBarrier>;
        std::vector<StepVariant> steps;
    };

    struct RGMemoryBlock
    {
        int id;
        size_t size = 0;
        int lastUsePass = -1;
        rhi::RHIHeap *heap = nullptr;
    };

    class RenderGraph
    {
        RGResourceID m_nextId = 0;
        std::unordered_map<RGResourceID, RGTextureDesc> m_textures;
        std::unordered_map<RGResourceID, RGBufferDesc> m_buffers;
        std::unordered_map<RGResourceID, rhi::RHITexture *> m_allocatedTextures;
        std::unordered_map<RGResourceID, RGAllocatedBuffer> m_allocatedBuffers;
        std::vector<RenderPass> m_passes;
        Graph m_depGraph;
        std::vector<NodeID> m_executionOrder;
        std::vector<RGMemoryBlock> m_memoryBlocks;
        std::unordered_map<RGResourceID, int> m_aliasAssignments;

    public:
        RGTextureID declareTexture(RGTextureDesc desc);
        RGBufferID declareBuffer(RGBufferDesc desc);
        void addPass(RenderPass pass);
        void compile();
        void dryRun();
        void allocateTextures(std::shared_ptr<rhi::RHIDevice> device, uint32_t frameWidth, uint32_t frameHeight);
        void allocateBuffers(std::shared_ptr<rhi::RHIDevice> device);
        void reallocateFrameTextures(std::shared_ptr<rhi::RHIDevice> device, uint32_t frameWidth, uint32_t frameHeight);
        void deleteTextures(std::shared_ptr<rhi::RHIDevice> device);
        void deleteBuffers(std::shared_ptr<rhi::RHIDevice> device);
        const RGResources buildResources();
        void bindPassResources(const RGResources &resources);

        const RGCompiledFrameGraph compileFrameGraph();
        void executeFrameGraph(const RGCompiledFrameGraph &compiledGraph, rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings, rhi::RHITimer *timer, uint32_t frameIdx = 0);
        std::vector<RGValidationError> validate();
        void drawImGui();
        std::unordered_map<RGResourceID, RGLifetime> computeLifetimes();
        std::unordered_map<RGResourceID, int> computeAliases(uint32_t frameWidth, uint32_t frameHeight);

    private:
        void drawLifetimeTimeline();
        bool isDepthFormat(rhi::TextureDesc::ImageFormat format);
        bool isWrittenByAnyPass(RGResourceID id);
        bool isSharedStorageBuffer(RGResourceID id);
        int writerOutputIndex(int passIdx, RGTextureID tid);
        int readerInputIndex(int passIdx, RGTextureID tid);
    };
} // namespace nitro::renderer

#pragma once
#include "nitro-frame/resources/resource-types.h"
#include "nitro-frame/resources/resource-manager.h"
#include "nitro-rhi/rhi-utils.h"
#include "nitro-core/graph.h"
#include "nitro-core/types/pool.h"
#include "nitro-core/utils.h"
#include <deque>
#include "nitro-frame/graph/blackboard.h"

namespace nitro::frame
{
    constexpr int kExternalProducer = -1;
    class RenderGraphBuilder;

    struct PassContext
    {
        rhi::RHICommandBuffer &cmd;
        const ResourceManager &resources;
        const RenderView &view;
        ResourceFrameInfo frame;
    };
    // TODO: Create a PassResource to block passes from accessing resources they did not declare
    using RenderPassExecute = std::function<void(PassContext &)>;

    struct LogicalKey
    {
        ResourceKey key;
        bool history = false;
        bool operator==(const LogicalKey &) const = default;
    };

    struct LogicalKeyHash
    {
        size_t operator()(const LogicalKey &k) const
        {
            size_t seed = ResourceKeyHash{}(k.key);
            hashCombine(seed, k.history);
            return seed;
        }
    };
    class RenderPass
    {
    public:
        RenderPass(uint32_t index, std::string name, RenderGraphBuilder &renderGraph, RenderPassExecute execute) : m_index(index), m_name(name), m_execute(std::move(execute)), m_renderGraph(renderGraph) {};

        RenderPass &read(const LogicalTextureHandle &handle, const rhi::ResourceState &state, rhi::TextureSubresource range = rhi::TextureSubresource{});

        RenderPass &read(const LogicalBufferHandle &handle, const rhi::ResourceState &state);

        RenderPass &write(const LogicalTextureHandle &handle, const rhi::ResourceState &state, rhi::TextureSubresource range = rhi::TextureSubresource{});

        RenderPass &write(const LogicalBufferHandle &handle, const rhi::ResourceState &state);

        const std::vector<TextureAccess> &getTextureAccess() const { return m_textures; }
        const std::vector<BufferAccess> &getBufferAccess() const { return m_buffers; }
        const RenderPassExecute &getExecute() const { return m_execute; }
        const std::string &name() const { return m_name; }
        uint32_t index() const { return m_index; }

    private:
        uint32_t m_index;
        std::string m_name;
        std::vector<TextureAccess> m_textures;
        std::vector<BufferAccess> m_buffers;
        RenderPassExecute m_execute;

        RenderGraphBuilder &m_renderGraph;
    };

    struct GraphExecuteInfo
    {
        rhi::RHICommandBuffer &cmd;
        const RenderView &view;
        ResourceFrameInfo frameInfo;
        rhi::RHITimer *timer = nullptr;
    };
    class RenderGraphBuilder
    {

    public:
        explicit RenderGraphBuilder(ResourceManager &resourceManager) : m_resourceManager(resourceManager) {};
        LogicalTextureHandle createTexture(const std::string name, const rhi::TextureDesc &desc, const ResourceSize &size, const GPUResourceOptions &options);
        LogicalTextureHandle createTexture(const PhysicalTextureHandle handle);

        LogicalBufferHandle createBuffer(const std::string name, const rhi::BufferDesc &desc, const ResourceSize &size, const GPUResourceOptions &options);
        LogicalBufferHandle createBuffer(const PhysicalBufferHandle handle);

        TemporalLogicalTexture createTemporalTexture(const std::string name, const rhi::TextureDesc &desc, const ResourceSize &size, const GPUResourceOptions &options);
        TemporalLogicalTexture createTemporalTexture(const PhysicalTextureHandle handle);

        TemporalLogicalBuffer createTemporalBuffer(const std::string name, const rhi::BufferDesc &desc, const ResourceSize &size, const GPUResourceOptions &option);
        TemporalLogicalBuffer createTemporalBuffer(const PhysicalBufferHandle handle);

        LogicalTextureHandle importTexture(const std::string &name, rhi::RHITextureHandle rhiHandle);
        LogicalBufferHandle importBuffer(const std::string &name, rhi::RHIBufferHandle rhiHandle);

        LogicalTexture *get(const LogicalTextureHandle &handle);
        LogicalBuffer *get(const LogicalBufferHandle &handle);

        RenderPass &addRenderPass(std::string name, RenderPassExecute execute);
        size_t structureHash() const;
        CompiledRenderGraph compile();

        const PhysicalTextureLifetime &lifetimes() const
        {
            return m_lifetime;
        }
        const std::vector<uint32_t> &executionOrder() const { return m_executionOrder; }
        uint32_t orderOf(const std::string &passName) const;

        Blackboard &blackboard() { return m_blackboard; }
        const Blackboard &blackboard() const { return m_blackboard; }
        void execute(const CompiledRenderGraph &compiledGraph, const GraphExecuteInfo &info);

    private:
        LogicalTextureHandle addLogical(const PhysicalTextureHandle handle, bool history);
        LogicalBufferHandle addLogical(const PhysicalBufferHandle handle, bool history);

        const PhysicalTextureLifetime calculateLifetimes() const;
        CompiledRenderGraph::Step buildRenderPassStep(int passIdx, int order);

    private:
        Graph m_graph;
        Blackboard m_blackboard;
        ResourceManager &m_resourceManager;
        ResourcePool<LogicalTexture, LogicalTextureTag> m_logicalTextures;
        ResourcePool<LogicalBuffer, LogicalBufferTag> m_logicalBuffers;

        std::deque<RenderPass> m_renderPasses;

        std::unordered_map<std::string, uint32_t> m_passNameToPassIdx;

        std::unordered_map<LogicalKey, LogicalTextureHandle, LogicalKeyHash> m_logicalTextureLookup;
        std::unordered_map<LogicalKey, LogicalBufferHandle, LogicalKeyHash> m_logicalBufferLookup;

        std::vector<uint32_t> m_executionOrder;
        PhysicalTextureLifetime m_lifetime;
    };
} // namespace nitro::frame

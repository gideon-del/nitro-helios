#pragma once
#include "nitro-frame/resources/resource-types.h"
#include "nitro-frame/resources/resource-manager.h"
#include "nitro-rhi/rhi-utils.h"
#include "nitro-core/graph.h"
#include <deque>

namespace nitro::frame
{
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

    class RenderPass
    {
    public:
        RenderPass(uint32_t index, std::string name, RenderGraphBuilder &renderGraph);

        RenderPass &read(const LogicalTextureHandle &handle, const rhi::ResourceState &state, rhi::TextureSubresource range = rhi::TextureSubresource{})
        {
            return read(handle, state, false, range);
        };
        RenderPass &read(const TemporalLogicalTexture &temporal, const rhi::ResourceState &state, rhi::TextureSubresource range = rhi::TextureSubresource{});

        RenderPass &read(const TemporalLogicalBuffer &temporal, const rhi::ResourceState &state);
        RenderPass &read(const LogicalBufferHandle &handle, const rhi::ResourceState &state)
        {
            return read(handle, state, false);
        };

        RenderPass &write(const LogicalTextureHandle &handle, const rhi::ResourceState &state, rhi::TextureSubresource range = rhi::TextureSubresource{});
        RenderPass &write(const TemporalLogicalTexture &temporal, const rhi::ResourceState &state, rhi::TextureSubresource range = rhi::TextureSubresource{});

        RenderPass &write(const TemporalLogicalBuffer &temporal, const rhi::ResourceState &state);
        RenderPass &write(const LogicalBufferHandle &handle, const rhi::ResourceState &state);

        RenderPass &readHistory(const TemporalLogicalBuffer &temporal, const rhi::ResourceState &state);
        RenderPass &readHistory(const TemporalLogicalTexture &temporal, const rhi::ResourceState &state, rhi::TextureSubresource range = rhi::TextureSubresource{});

        RenderPass &setExecuteFn(RenderPassExecute execute);

        const std::vector<TextureAccess> &getTextureAccess() const { return m_textures; }
        const std::vector<BufferAccess> &getBufferAccess() const { return m_buffers; }
        const RenderPassExecute &getExecute() { return m_execute; }
        const std::string &name() const { return m_name; }
        const uint32_t index() const { return m_index; }

    private:
        RenderPass &read(const LogicalTextureHandle &handle, const rhi::ResourceState &state, bool history, rhi::TextureSubresource range);
        RenderPass &read(const LogicalBufferHandle &handle, const rhi::ResourceState &state, bool history);

    private:
        uint32_t m_index;
        std::string m_name;
        std::vector<TextureAccess> m_textures;
        std::vector<BufferAccess> m_buffers;
        RenderPassExecute m_execute;

        RenderGraphBuilder &m_renderGraph;
    };

    class RenderGraphBuilder
    {

    public:
        RenderGraphBuilder(ResourceManager &resourceManager);
        LogicalTextureHandle createTexture(const std::string name, const rhi::TextureDesc &desc, const ResourceSize &size, const GPUResourceOptions &options);
        LogicalTextureHandle createTexture(const PhysicalTextureHandle handle);

        LogicalBufferHandle createBuffer(const std::string name, const rhi::BufferDesc &desc, const ResourceSize &size, const GPUResourceOptions &options);
        LogicalBufferHandle createBuffer(const PhysicalBufferHandle handle);

        TemporalLogicalTexture createTemporalTexture(const std::string name, const rhi::TextureDesc &desc, const ResourceSize &size, const GPUResourceOptions &options);
        TemporalLogicalTexture createTemporalTexture(const PhysicalTextureHandle handle);

        TemporalLogicalBuffer createTemporalBuffer(const std::string name, const rhi::BufferDesc &desc, const ResourceSize &size, const GPUResourceOptions &option);
        TemporalLogicalBuffer createTemporalBuffer(const PhysicalBufferHandle handle);

        LogicalTextureHandle importTexture(const std::string &name, rhi::RHITextureHandle rhiHandle);
        LogicalBufferHandle importBuffer(const std::string &name, rhi::RHIBufferHandle handle);

        LogicalTexture *getTexture(const LogicalTextureHandle &handle);
        LogicalBuffer *getBuffer(const LogicalBufferHandle &handle);

        RenderPass &addRenderPass(std::string name);

        const CompiledRenderGraph compile();
        void printResourceFlow();
        void printPassOrder();
        void printResourceLifetime();

        const PhysicalTextureLifetime &lifetimes() const
        {
            return m_lifetime;
        }
        const std::vector<uint32_t> &executionOrder() const { return m_executionOrder; }
        uint32_t orderOf(const std::string &passName) const;

    private:
        LogicalTextureHandle createTexture(const std::string &name, bool history);
        LogicalBufferHandle createBuffer(const std::string &name, bool history);
        const PhysicalTextureLifetime calculateLifetimes() const;
        const CompiledRenderGraph::Step buildRenderPassStep(int passIdx);

    private:
        Graph m_graph;
        ResourceManager &m_resourceManager;
        std::vector<LogicalTexture> m_logicalTextures;
        std::vector<LogicalBuffer> m_logicalBuffers;

        std::deque<RenderPass> m_renderPasses;
        std::unordered_map<std::string, uint32_t> m_nameToPassIndex;

        std::vector<uint32_t> m_executionOrder;
        PhysicalTextureLifetime m_lifetime;
    };
} // namespace nitro::frame

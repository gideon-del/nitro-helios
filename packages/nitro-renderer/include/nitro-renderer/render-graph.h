#pragma once
#include <nitro-rhi/rhi.h>
#include <cstdint>
#include <functional>
#include <nitro-renderer/context.h>
#include <nitro-renderer/settings.h>
#include "graph.h"
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
        bool transient = true;
        bool isStorage = false;
    };
    struct RGBufferDesc
    {
        std::string name;
        uint32_t size;
        rhi::BufferDesc::Usage usage;
        bool transient = true;
    };
    struct RGValidationError
    {
        std::string passName;
        std::string textureName;
        std::string message;
    };
    struct RGResources
    {
        std::unordered_map<RGResourceID, rhi::RHITexture *> allocatedTextures;
        std::unordered_map<RGResourceID, rhi::RHIBuffer *> allocatedBuffers;
        rhi::RHITexture *getTexture(RGResourceID id) const
        {
            if (!allocatedTextures.count(id))
                return nullptr;
            return allocatedTextures.at(id);
        }
        rhi::RHIBuffer *getBuffer(RGResourceID id) const
        {
            if (!allocatedBuffers.count(id))
                return nullptr;
            return allocatedBuffers.at(id);
        }
    };

    struct RenderPass
    {
        std::string name;

        std::vector<RGTextureID> reads, writes;
        std::vector<RGBufferID> readBufs, writeBufs;
        std::function<void(const RGResources &resources)> bind;
        std::function<void(rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)> execute;
    };

    class RenderGraph
    {
        RGResourceID m_nextId = 0;
        std::unordered_map<RGResourceID, RGTextureDesc> m_textures;
        std::unordered_map<RGResourceID, RGBufferDesc> m_buffers;
        std::unordered_map<RGResourceID, rhi::RHITexture *> m_allocatedTextures;
        std::unordered_map<RGResourceID, rhi::RHIBuffer *> m_allocatedBuffers;
        std::vector<RenderPass> m_passes;
        Graph m_depGraph;
        std::vector<NodeID> m_executionOrder;

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
        void execute(rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings, const RGResources &resources);
        std::vector<RGValidationError> validate();

    private:
        bool isDepthFormat(rhi::TextureDesc::ImageFormat format);
        bool isWrittenByAnyPass(RGResourceID id);
        bool isSharedStorageBuffer(RGResourceID id);
    };
} // namespace nitro::renderer

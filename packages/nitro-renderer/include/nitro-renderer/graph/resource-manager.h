#pragma once
#include "nitro-renderer/graph/resource-types.h"
#include "nitro-renderer/graph/multi-buffer.h"
#include "nitro-core/types/pool.h"
#include <unordered_map>
#include <unordered_set>
namespace nitro::renderer
{
    struct ViewBuildInfo
    {
        RenderViewHandle handle;
        Extent2D extent;
        size_t structureHash;
        const PhysicalTextureLifetime &lifetimes;
    };
    class ResourceManager
    {
        template <typename T, typename Desc>
        struct PhysicalResource
        {
            ResourceKey key;
            ResourceInstanceType instanceType;
            ResourceLifetimeType lifetime;
            ResourceScope scope;
            Desc descriptor;
            ResourceSize size;
            uint32_t historyCount = 2;
            MultiBuffer<T> resources;
            Extent2D builtExtent{};
            bool dirty = true;
        };

        using PhysicalTexture = PhysicalResource<rhi::RHITextureHandle, rhi::TextureDesc>;
        using PhysicalBuffer = PhysicalResource<rhi::RHIBufferHandle, rhi::BufferDesc>;

        ResourcePool<PhysicalTexture, PhysicalTextureTag> m_textures;
        ResourcePool<PhysicalBuffer, PhysicalBufferTag> m_buffers;

        // Pending Builds for global resources
        std::unordered_set<PhysicalTextureHandle, PhysicalTextureHandleHash> m_pendingTextures;
        std::unordered_set<PhysicalBufferHandle, PhysicalBufferHandleHash> m_pendingBuffers;

        // View Persistent resources
        std::unordered_map<RenderViewHandle, std::unordered_set<PhysicalTextureHandle, PhysicalTextureHandleHash>, RenderViewHandleHash> m_viewRelativeTexture;
        std::unordered_map<RenderViewHandle, std::unordered_set<PhysicalBufferHandle, PhysicalBufferHandleHash>, RenderViewHandleHash> m_viewRelativeBuffer;

        // Lookup
        std::unordered_map<ResourceKey, PhysicalTextureHandle, ResourceKeyHash> m_resourceKeyToTexture;
        std::unordered_map<ResourceKey, PhysicalBufferHandle, ResourceKeyHash> m_resourceKeyToBuffer;

        // Transient resource plan

        struct CachedAliasPlan
        {
            AliasPlan aliasPlan;
            size_t inputHash = 0;
            bool valid = false;
        };
        std::unordered_map<RenderViewHandle, CachedAliasPlan, RenderViewHandleHash> m_aliasPlans;

        // View based heaps

        using HeapStorage = std::unordered_map<HeapId, rhi::RHIHeapHandle>;

        std::unordered_map<RenderViewHandle, HeapStorage, RenderViewHandleHash> m_viewToHeaps;

        rhi::RHIDevice &m_device;

    public:
        explicit ResourceManager(rhi::RHIDevice &device) : m_device(device) {};

        PhysicalTextureHandle createTexture(const std::string name, const rhi::TextureDesc &desc, const ResourceSize &size, const GPUResourceOptions &options);
        PhysicalBufferHandle createBuffer(const std::string name, const rhi::BufferDesc &desc, const ResourceSize &size, const GPUResourceOptions &options);

        PhysicalTextureHandle importTexture(const std::string &name, rhi::RHITextureHandle rhiHandle);
        void updateExternalTexture(const PhysicalTextureHandle &handle, const rhi::RHITextureHandle &rhiHandle);

        PhysicalBufferHandle importBuffer(const std::string &name, rhi::RHIBufferHandle handle);
        void updateExternalBuffer(const PhysicalBufferHandle &handle, const rhi::RHIBufferHandle &rhiHandle);

        rhi::RHITexture *get(const PhysicalTextureHandle &handle, const ResourceFrameInfo &frameInfo);
        rhi::RHIBuffer *get(const PhysicalBufferHandle &handle, const ResourceFrameInfo &frameInfo);

        rhi::RHITexture *previous(const PhysicalTextureHandle &handle, const ResourceFrameInfo &frameInfo);
        rhi::RHIBuffer *previous(const PhysicalBufferHandle &handle, const ResourceFrameInfo &frameInfo);

        rhi::RHITexture *getTexture(const ResourceKey &key, const ResourceFrameInfo &frameInfo);
        rhi::RHIBuffer *getBuffer(const ResourceKey &key, const ResourceFrameInfo &frameInfo);

        void build();
        bool buildViewResources(const ViewBuildInfo &info);
        void destroyViewResources(const RenderViewHandle viewHandle);

    private:
        void clearResource(PhysicalBuffer &buffer);
        void clearResource(PhysicalTexture &texture);
        void destroyResource(PhysicalTextureHandle handle);
        void destroyResource(PhysicalBufferHandle handle);

        void updateTextureDeclaration(
            PhysicalTextureHandle handle,
            PhysicalTexture &texture,
            const rhi::TextureDesc &desc,
            const ResourceSize &size,
            const GPUResourceOptions &options);

        void updateBufferDeclaration(
            PhysicalBufferHandle handle,
            PhysicalBuffer &buffer,
            const rhi::BufferDesc &desc,
            const ResourceSize &size,
            const GPUResourceOptions &options);

        void enforceOptions(const GPUResourceOptions &options, const ResourceSize &size);

        uint32_t getResourceCount(const ResourceInstanceType &type, uint32_t historyCount);
        std::vector<PhysicalTextureHandle> sortLifetimeHandle(const PhysicalTextureLifetime &lifetime);
        bool buildPersistentResource(const PhysicalTextureHandle &handle, const Extent2D size = {});
        bool buildPersistentResource(const PhysicalBufferHandle &handle, const Extent2D size = {});

        bool buildPersistentViewTextures(const RenderViewHandle &viewHandle, const Extent2D &extent);
        bool buildPersistentViewBuffers(const RenderViewHandle &viewHandle, const Extent2D &extent);

        void rebuildTransientViewTextures(const RenderViewHandle &viewHandle, const Extent2D &extent, const PhysicalTextureLifetime &lifetime);

        std::optional<PhysicalTextureHandle> lookupTextureHandle(const ResourceKey &key);
        std::optional<PhysicalBufferHandle> lookupBufferHandle(const ResourceKey &key);

        AliasPlan calculateAliasPlan(const PhysicalTextureLifetime &lifetimes, const Extent2D &size);
        std::vector<AliasCommand> diffAliasPlan(const AliasPlan &newPlan);

        void applyAlias(const CreateHeapCommand &command, RenderViewHandle view);
        void applyAlias(const CreateAliasTextureCommand &command, RenderViewHandle view);
        void applyAlias(const DeleteAliasTextureCommand &command);
        void applyAlias(const DeleteHeapCommand &command, RenderViewHandle view);
    };
} // namespace nitro::renderer

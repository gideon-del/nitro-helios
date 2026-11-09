#pragma once
#include "nitro-rhi/rhi-device.h"
#include "nitro-core/types/pool.h"
#include "nitro-core/types/size.h"
#include "nitro-core/utils.h"
#include "nitro-rhi/rhi-texture.h"
#include "nitro-frame/view/render-view.h"
#include <variant>

namespace nitro::frame
{
    struct ResourceSize
    {

        enum class Type
        {
            Absolute,
            ViewRelative,
            TileRelative
        } type = Type::Absolute;
        uint32_t width = 1, height = 1;
        float scale = 1.0f;

        size_t bytesPerTile = 1;
        size_t tileSize = 16;

        uint32_t calculateTextureWidth(const uint32_t screenWidth);
        uint32_t calculateTextureHeight(const uint32_t screenHeight);
        size_t calculateBufferSize(const uint32_t screenWidth, const uint32_t screenHeight);
        static ResourceSize bufferAbsolute(size_t bytes);
        static ResourceSize bufferViewRelative(size_t bytesPerPixel, float scale = 1.0f);
        static ResourceSize bufferTileRelative(uint32_t tileSize, size_t bytesPerTile);

        static ResourceSize textureAbsolute(uint32_t w, uint32_t h);
        static ResourceSize textureViewRelative(float scale = 1.0f);

        bool operator==(const ResourceSize &) const = default;
    };
    struct PhysicalTextureTag
    {
    };

    using PhysicalTextureHandle = Handle<PhysicalTextureTag>;
    using PhysicalTextureHandleHash = HandleHash<PhysicalTextureTag>;

    struct PhysicalBufferTag
    {
    };

    using PhysicalBufferHandle = Handle<PhysicalBufferTag>;
    using PhysicalBufferHandleHash = HandleHash<PhysicalBufferTag>;

    struct ResourceVersion
    {
        std::optional<int> producer{
            std::nullopt};
        std::vector<int> consumers;
    };

    enum class ResourceLifetimeType
    {
        Persistent,
        Transient
    };

    enum class ResourceInstanceType
    {
        Single,
        PerFrame,
        Temporal
    };

    enum class ResourceScope
    {
        Global,
        View,
        External
    };

    struct GPUResourceOptions
    {
        ResourceLifetimeType lifetime = ResourceLifetimeType::Persistent;
        ResourceInstanceType instanceType = ResourceInstanceType::Single;
        ResourceScope scope = ResourceScope::Global;
        RenderViewHandle viewHandle = {};
        uint32_t historyCount = 2;
    };

    struct ResourceKey
    {
        std::string name;
        RenderViewHandle viewHandle = {};
        bool operator==(const ResourceKey &) const = default;
    };

    struct ResourceKeyHash
    {
        size_t operator()(const ResourceKey &key) const
        {
            size_t seed = 0;

            hashCombine(seed, std::hash<std::string>{}(key.name));
            hashCombine(seed, RenderViewHandleHash{}(key.viewHandle));

            return seed;
        }
    };

    template <typename T>
    struct LogicalResource
    {

        std::string name;
        T physicalHandle;
        std::vector<ResourceVersion> versions;
        bool history = false;
    };

    using LogicalTexture = LogicalResource<PhysicalTextureHandle>;
    using LogicalBuffer = LogicalResource<PhysicalBufferHandle>;

    using LogicalTextureHandle = Handle<LogicalTexture>;
    using LogicalBufferHandle = Handle<LogicalBuffer>;

    template <typename T>
    struct TemporalLogicalResource
    {
        T current;
        T previous;
    };

    using TemporalLogicalTexture = TemporalLogicalResource<LogicalTextureHandle>;
    using TemporalLogicalBuffer = TemporalLogicalResource<LogicalBufferHandle>;

    struct ResourceLifetimeV2
    {
        std::string name;
        int lastUse = -1;
        int createdAt = -1;
    };
    struct TransientEntry
    {
        PhysicalTextureHandle handle;
        int first, last;
        rhi::MemoryRequirements requirements;
    };
    using PhysicalTextureLifetime = std::unordered_map<PhysicalTextureHandle, ResourceLifetimeV2, PhysicalTextureHandleHash>;
    struct HeapId
    {
        uint64_t key;

        bool operator==(const HeapId &) const = default;
    };
    struct HeapIdHash
    {
        size_t operator()(const HeapId &id) const
        {
            return std::hash<uint64_t>()(id.key);
        }
    };
    struct AliasGroup
    {
        HeapId id;
        rhi::MemoryRequirements requirement;
        std::vector<TransientEntry> textures;
    };

    struct AliasPlan
    {
        std::unordered_map<HeapId, AliasGroup, HeapIdHash> groups;
        std::unordered_map<PhysicalTextureHandle, HeapId, PhysicalTextureHandleHash> textureToHeap;
    };

    struct ResourceFrameInfo
    {
        uint32_t rhiFrame = 0;
        uint32_t viewFrame = 0;
        RenderViewHandle viewHandle = {};
    };

    struct CreateHeapCommand
    {
        HeapId id;
        rhi::MemoryRequirements requirement;
    };
    struct CreateAliasTextureCommand
    {
        HeapId id;
        PhysicalTextureHandle texture;
    };
    struct DeleteAliasTextureCommand
    {

        PhysicalTextureHandle texture;
    };
    struct DeleteHeapCommand
    {
        HeapId id;
    };

    using AliasCommand = std::variant<
        CreateHeapCommand,
        CreateAliasTextureCommand,
        DeleteAliasTextureCommand,
        DeleteHeapCommand>;

} // namespace nitro::frame

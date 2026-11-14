#include "nitro-frame/resources/resource-manager.h"
#include "nitro-rhi/rhi-constants.h"
#include "nitro-core/utils.h"
#include <algorithm>
#include "nitro-frame/resources/alias-planner.h"
namespace nitro::frame
{
    template <typename R>
    static bool needsRebuild(const R &r, const Extent2D &extent)
    {

        if (r.dirty || r.resources.empty())
            return true;
        return r.size.type != ResourceSize::Type::Absolute && r.builtExtent != extent;
    }

    template <typename R>
    static uint32_t instanceIndex(const R &r, const ResourceFrameInfo &info, bool previous = false)
    {
        switch (r.instanceType)
        {
        case ResourceInstanceType::PerFrame:
            return info.rhiFrame;
        case ResourceInstanceType::Temporal:
            return previous ? info.viewFrame + r.historyCount - 1
                            : info.viewFrame;
        default:
            return 0;
        }
    }

    static size_t aliasInputHash(size_t structureHash, Extent2D extent)
    {
        size_t seed = 0;
        hashCombine(seed, structureHash);
        hashCombine(seed, extent.width);
        hashCombine(seed, extent.height);
        return seed;
    }
    void ResourceManager::enforceOptions(const GPUResourceOptions &options, const ResourceSize &size)
    {

        assert(options.scope != ResourceScope::External && "External Resources can only be imported or updated");
        if (options.scope == ResourceScope::View)
        {
            assert(options.viewHandle.isValid() && "View scope resources must have a valid view handle");
        }

        if (options.scope == ResourceScope::Global)
        {
            assert(size.type == ResourceSize::Type::Absolute && "Global resources must have absolute sizing");
            assert(options.lifetime != ResourceLifetimeType::Transient && "Only View scoped resources can be transient");
            assert(!options.viewHandle.isValid() && "Global resource must have no view handle");
        }

        if (options.instanceType == ResourceInstanceType::Temporal)
        {
            assert(options.historyCount > 1 && "History Count must be greater than one for temporal resources");
        }

        if (options.lifetime == ResourceLifetimeType::Transient)
            assert(options.instanceType == ResourceInstanceType::Single && "Transients can't keep history or per-frame copies");

        if (options.instanceType == ResourceInstanceType::Temporal)
            assert(options.scope == ResourceScope::View && "Temporal history follows a view's clock");
    };

    void ResourceManager::updateTextureDeclaration(
        PhysicalTextureHandle handle,
        PhysicalTexture &texture,
        const rhi::TextureDesc &desc,
        const ResourceSize &size,
        const GPUResourceOptions &options)
    {
        assert(texture.info.options.scope == options.scope);
        assert(texture.info.options.lifetime == options.lifetime);
        assert(texture.info.options.instanceType == options.instanceType);

        bool changed =
            texture.info.descriptor != desc ||
            texture.info.size != size ||
            texture.info.options.historyCount != options.historyCount;

        if (!changed)
            return;

        texture.info.descriptor = desc;
        texture.info.size = size;
        texture.info.options.historyCount = options.historyCount;
        texture.dirty = true;

        if (texture.info.options.lifetime == ResourceLifetimeType::Persistent)
        {
            if (texture.info.options.scope == ResourceScope::Global)
            {
                m_pendingTextures.insert(handle);
            }
            else
            {
                m_viewRelativeTexture[options.viewHandle].insert(handle);
            }
        };
    }
    void ResourceManager::updateBufferDeclaration(
        PhysicalBufferHandle handle,
        PhysicalBuffer &buffer,
        const rhi::BufferDesc &desc,
        const ResourceSize &size,
        const GPUResourceOptions &options)
    {
        assert(buffer.info.options.scope == options.scope);
        assert(buffer.info.options.lifetime == options.lifetime);
        assert(buffer.info.options.instanceType == options.instanceType);

        bool changed =
            buffer.info.descriptor != desc ||
            buffer.info.size != size ||
            buffer.info.options.historyCount != options.historyCount;

        if (!changed)
            return;

        buffer.info.descriptor = desc;
        buffer.info.size = size;
        buffer.info.options.historyCount = options.historyCount;
        buffer.dirty = true;
        if (buffer.info.options.lifetime == ResourceLifetimeType::Persistent)
        {
            if (buffer.info.options.scope == ResourceScope::Global)
            {
                m_pendingBuffers.insert(handle);
            }
            else
            {
                m_viewRelativeBuffer[options.viewHandle].insert(handle);
            }
        };
    }

    PhysicalTextureHandle ResourceManager::createTexture(const std::string name, const rhi::TextureDesc &desc, const ResourceSize &size, const GPUResourceOptions &options)
    {
        enforceOptions(options, size);

        auto key = ResourceKey{
            .name = name,
            .viewHandle = options.viewHandle};

        auto it = m_resourceKeyToTexture.find(key);

        if (it != m_resourceKeyToTexture.end())
        {
            auto handle = it->second;
            auto texture = m_textures.get(handle);

            if (texture)
            {
                updateTextureDeclaration(
                    handle, *texture, desc, size, options);

                return handle;
            }
        }

        auto physicalTexture = PhysicalTexture{
            .info = {
                .key = key,
                .descriptor = desc,
                .size = size,
                .options = options}

        };

        auto handle = m_textures.emplace(physicalTexture);

        if (options.scope == ResourceScope::View)
            m_viewRelativeTexture[options.viewHandle].insert(handle);
        else if (options.lifetime == ResourceLifetimeType::Persistent)
            m_pendingTextures.insert(handle);

        m_resourceKeyToTexture[key] = handle;

        return handle;
    };

    PhysicalBufferHandle ResourceManager::createBuffer(const std::string name, const rhi::BufferDesc &desc, const ResourceSize &size, const GPUResourceOptions &options)
    {
        assert(options.lifetime == ResourceLifetimeType::Persistent && "Buffers can only be persistent");
        enforceOptions(options, size);

        auto key = ResourceKey{
            .name = name,
            .viewHandle = options.viewHandle};

        auto it = m_resourceKeyToBuffer.find(key);

        if (it != m_resourceKeyToBuffer.end())
        {
            auto handle = it->second;
            auto buffer = m_buffers.get(handle);

            if (buffer)
            {
                updateBufferDeclaration(
                    handle, *buffer, desc, size, options);

                return handle;
            }
        }

        auto physicalBuffer = PhysicalBuffer{
            .info = {
                .key = key,
                .descriptor = desc,
                .size = size,
                .options = options}};

        auto handle = m_buffers.emplace(physicalBuffer);

        if (options.scope == ResourceScope::Global)
        {
            m_pendingBuffers.insert(handle);
        }
        else
        {
            m_viewRelativeBuffer[options.viewHandle].insert(handle);
        }

        m_resourceKeyToBuffer[key] = handle;

        return handle;
    };

    void ResourceManager::clearResource(PhysicalBuffer &buffer)
    {
        buffer.resources.forEach([&](const rhi::RHIBufferHandle &handle)
                                 { m_device.destroy(handle); });
        buffer.resources.clear();
    }
    void ResourceManager::clearResource(PhysicalTexture &texture)
    {
        texture.resources.forEach([&](const rhi::RHITextureHandle &handle)
                                  { m_device.destroy(handle); });
        texture.resources.clear();
    }

    void ResourceManager::build()
    {
        for (auto &physicalHandle : m_pendingTextures)
        {
            buildPersistentResource(physicalHandle);
        }

        m_pendingTextures.clear();

        for (auto &handle : m_pendingBuffers)
        {
            buildPersistentResource(handle);
        }

        m_pendingBuffers.clear();
    }

    uint32_t ResourceManager::getResourceCount(const ResourceInstanceType &type, uint32_t historyCount)
    {
        switch (type)
        {
        case ResourceInstanceType::Single:
            return 1;
        case ResourceInstanceType::PerFrame:
            return rhi::kMaxFramesInFlight;
        case ResourceInstanceType::Temporal:
            return historyCount;
        default:
            return 1;
        }
    }

    bool ResourceManager::buildPersistentResource(const PhysicalTextureHandle &handle, const Extent2D size)
    {
        auto texture = m_textures.get(handle);
        if (!texture)
            return false;
        if (texture->info.options.lifetime == ResourceLifetimeType::Transient)
            return false;
        if (!needsRebuild(*texture, size))
            return false;
        clearResource(*texture);

        auto count = getResourceCount(texture->info.options.instanceType, texture->info.options.historyCount);

        texture->resources.create(count, [&](uint32_t index)
                                  {
                                      auto desc = texture->info.descriptor;
                                      desc.size.width = texture->info.size.calculateTextureWidth(size.width);
                                      desc.size.height = texture->info.size.calculateTextureHeight(size.height);
                                    

                                      return m_device.create(desc); });

        texture->dirty = false;
        texture->builtExtent = size;
        return true;
    }
    bool ResourceManager::buildPersistentResource(const PhysicalBufferHandle &handle, const Extent2D size)
    {
        auto buffer = m_buffers.get(handle);
        if (!buffer)
            return false;

        if (!needsRebuild(*buffer, size))
            return false;
        // Delete old resource
        clearResource(*buffer);

        auto count = getResourceCount(buffer->info.options.instanceType, buffer->info.options.historyCount);

        buffer->resources.create(count, [&](uint32_t index)
                                 {
                                      auto desc = buffer->info.descriptor;
                                      desc.size = buffer->info.size.calculateBufferSize(size.width,size.height);

                                      return m_device.create(desc); });

        buffer->builtExtent = size;
        buffer->dirty = false;
        return true;
    }

    bool ResourceManager::buildViewResources(const ViewBuildInfo &info)
    {
        bool historyLost = buildPersistentViewBuffers(info.handle, info.extent);
        historyLost |= buildPersistentViewTextures(info.handle, info.extent);
        rebuildTransientViewTextures(info);
        return historyLost;
    }

    bool ResourceManager::buildPersistentViewTextures(const RenderViewHandle &viewHandle, const Extent2D &extent)
    {
        auto it = m_viewRelativeTexture.find(viewHandle);

        if (it == m_viewRelativeTexture.end())
            return false;
        bool historyLost = false;
        for (auto &handle : it->second)
        {
            auto *texture = m_textures.get(handle);
            assert(texture && "view set holds a freed handle");
            if (!texture)
                continue;
            if (buildPersistentResource(handle, extent))
                historyLost |= texture->info.options.instanceType == ResourceInstanceType::Temporal;
        }

        return historyLost;
    }

    bool ResourceManager::buildPersistentViewBuffers(const RenderViewHandle &viewHandle, const Extent2D &extent)
    {
        auto it = m_viewRelativeBuffer.find(viewHandle);

        if (it == m_viewRelativeBuffer.end())
            return false;
        bool historyLost = false;
        for (auto &handle : it->second)
        {
            auto *buffer = m_buffers.get(handle);
            assert(buffer && "view set holds a freed handle");
            if (!buffer)
                continue;
            if (buildPersistentResource(handle, extent))
                historyLost |= buffer->info.options.instanceType == ResourceInstanceType::Temporal;
        }

        return historyLost;
    }

    void ResourceManager::destroyResource(PhysicalTextureHandle handle)
    {
        auto texture = m_textures.get(handle);
        if (!texture)
            return;

        if (texture->info.options.scope == ResourceScope::External)
            texture->resources.clear();
        else
            clearResource(*texture);

        if (auto it = m_viewRelativeTexture.find(texture->info.key.viewHandle); it != m_viewRelativeTexture.end())
            it->second.erase(handle);

        m_pendingTextures.erase(handle);
        m_resourceKeyToTexture.erase(texture->info.key);
        m_textures.free(handle);
    }

    void ResourceManager::destroyResource(PhysicalBufferHandle handle)
    {
        auto buffer = m_buffers.get(handle);
        if (!buffer)
            return;

        if (buffer->info.options.scope == ResourceScope::External)
            buffer->resources.clear();
        else
            clearResource(*buffer);

        if (auto it = m_viewRelativeBuffer.find(buffer->info.key.viewHandle); it != m_viewRelativeBuffer.end())
            it->second.erase(handle);

        m_pendingBuffers.erase(handle);
        m_resourceKeyToBuffer.erase(buffer->info.key);
        m_buffers.free(handle);
    }

    void ResourceManager::destroyViewResources(const RenderViewHandle viewHandle)
    {

        if (auto it = m_viewRelativeTexture.find(viewHandle); it != m_viewRelativeTexture.end())
        {
            auto handles = std::move(it->second);
            m_viewRelativeTexture.erase(it);
            for (auto h : handles)
                destroyResource(h);
        }

        if (auto it = m_viewRelativeBuffer.find(viewHandle); it != m_viewRelativeBuffer.end())
        {
            auto handles = std::move(it->second);
            m_viewRelativeBuffer.erase(it);
            for (auto h : handles)
                destroyResource(h);
        }

        if (auto it = m_viewToHeaps.find(viewHandle); it != m_viewToHeaps.end())
        {
            for (auto &[id, heap] : it->second)
                m_device.destroy(heap.handle);
            m_viewToHeaps.erase(it);
        }
        m_aliasPlans.erase(viewHandle);
    }

    rhi::RHITexture *ResourceManager::get(const PhysicalTextureHandle &handle, const ResourceFrameInfo &info)
    {
        auto *t = m_textures.get(handle);
        assert(t && !t->resources.empty() && "get() on a freed or unbuilt resource");
        return m_device.resolve(*t->resources.get(instanceIndex(*t, info)));
    }

    rhi::RHIBuffer *ResourceManager::get(const PhysicalBufferHandle &handle, const ResourceFrameInfo &info)
    {
        auto *b = m_buffers.get(handle);
        assert(b && !b->resources.empty() && "get() on a freed or unbuilt resource");
        return m_device.resolve(*b->resources.get(instanceIndex(*b, info)));
    }

    rhi::RHITexture *ResourceManager::previous(const PhysicalTextureHandle &handle, const ResourceFrameInfo &info)
    {
        auto *t = m_textures.get(handle);
        assert(t && !t->resources.empty() && t->info.options.instanceType == ResourceInstanceType::Temporal && "previous() on a non-temporal resource");
        return m_device.resolve(*t->resources.get(instanceIndex(*t, info, true)));
    }
    rhi::RHIBuffer *ResourceManager::previous(const PhysicalBufferHandle &handle, const ResourceFrameInfo &info)
    {
        auto *b = m_buffers.get(handle);
        assert(b && !b->resources.empty() && b->info.options.instanceType == ResourceInstanceType::Temporal && "previous() on a non-temporal resource");
        return m_device.resolve(*b->resources.get(instanceIndex(*b, info, true)));
    }

    std::optional<PhysicalTextureHandle> ResourceManager::lookupTextureHandle(const ResourceKey &key)
    {
        auto it = m_resourceKeyToTexture.find(key);
        if (it == m_resourceKeyToTexture.end())
            return std::nullopt;
        return it->second;
    }
    std::optional<PhysicalBufferHandle> ResourceManager::lookupBufferHandle(const ResourceKey &key)
    {
        auto it = m_resourceKeyToBuffer.find(key);
        if (it == m_resourceKeyToBuffer.end())
            return std::nullopt;
        return it->second;
    }

    rhi::RHITexture *ResourceManager::getTexture(const ResourceKey &key, const ResourceFrameInfo &info)
    {
        auto handle = lookupTextureHandle(key);
        return handle ? get(*handle, info) : nullptr;
    }
    rhi::RHIBuffer *ResourceManager::getBuffer(const ResourceKey &key, const ResourceFrameInfo &info)
    {
        auto handle = lookupBufferHandle(key);
        return handle ? get(*handle, info) : nullptr;
    }

    PhysicalTextureHandle ResourceManager::importTexture(const std::string &name, rhi::RHITextureHandle rhiHandle)
    {
        auto key = ResourceKey{.name = name};
        if (auto existing = lookupTextureHandle(key))
        {
            updateExternalTexture(*existing, rhiHandle);
            return *existing;
        }

        auto texture = PhysicalTexture{
            .info = {
                .key = key}};

        texture.info.options.instanceType = ResourceInstanceType::Single,
        texture.info.options.lifetime = ResourceLifetimeType::Persistent,
        texture.info.options.scope = ResourceScope::External;
        texture.resources.create(1, [&](uint32_t)
                                 { return rhiHandle; });
        texture.dirty = false;

        auto handle = m_textures.emplace(texture);
        m_resourceKeyToTexture[key] = handle;
        return handle;
    }

    PhysicalBufferHandle ResourceManager::importBuffer(const std::string &name, rhi::RHIBufferHandle rhiHandle)
    {
        auto key = ResourceKey{.name = name};
        if (auto existing = lookupBufferHandle(key))
        {
            updateExternalBuffer(*existing, rhiHandle);
            return *existing;
        }

        auto buffer = PhysicalBuffer{
            .info = {
                .key = key}};

        buffer.info.options.instanceType = ResourceInstanceType::Single,
        buffer.info.options.lifetime = ResourceLifetimeType::Persistent,
        buffer.info.options.scope = ResourceScope::External;
        buffer.resources.create(1, [&](uint32_t)
                                { return rhiHandle; });
        buffer.dirty = false;

        auto handle = m_buffers.emplace(buffer);
        m_resourceKeyToBuffer[key] = handle;
        return handle;
    }

    void ResourceManager::updateExternalTexture(const PhysicalTextureHandle &handle, const rhi::RHITextureHandle &rhiHandle)
    {
        auto *t = m_textures.get(handle);
        assert(t && t->info.options.scope == ResourceScope::External && "only imports can be re-bound");
        t->resources.clear();
        t->resources.create(1, [&](uint32_t)
                            { return rhiHandle; });
    }

    void ResourceManager::updateExternalBuffer(const PhysicalBufferHandle &handle, const rhi::RHIBufferHandle &rhiHandle)
    {
        auto *b = m_buffers.get(handle);
        assert(b && b->info.options.scope == ResourceScope::External && "only imports can be re-bound");
        b->resources.clear();
        b->resources.create(1, [&](uint32_t)
                            { return rhiHandle; });
    }

    void ResourceManager::rebuildTransientViewTextures(const ViewBuildInfo &info)
    {
        auto &cachedAlias = m_aliasPlans[info.handle];

        auto hash = aliasInputHash(info.structureHash, info.extent);

        if (hash == cachedAlias.inputHash && cachedAlias.valid)
            return;

        std::vector<TransientEntry> entries;

        for (auto &[handle, lifetime] : info.lifetimes)
        {
            auto texture = m_textures.get(handle);
            if (!texture || texture->info.options.lifetime != ResourceLifetimeType::Transient)
                continue;

            auto desc = texture->info.descriptor;

            desc.size.width = texture->info.size.calculateTextureWidth(info.extent.width);
            desc.size.height = texture->info.size.calculateTextureHeight(info.extent.height);

            auto requirement = m_device.textureMemoryRequirements(desc);

            entries.push_back({
                .handle = handle,
                .first = lifetime.createdAt,
                .last = lifetime.lastUse,
                .requirements = requirement,
            });
        }

        auto newPlan = planAliases(entries);

        auto commands = diffAliasPlan(newPlan, cachedAlias.aliasPlan, info.extent);

        for (auto &command : commands)
        {
            std::visit(
                overloaded{
                    [&](const DeleteAliasTextureCommand &c)
                    {
                        applyAlias(c);
                    },
                    [&](const DeleteHeapCommand &c)
                    {
                        applyAlias(c, info.handle);
                    },
                    [&](const CreateHeapCommand &c)
                    {
                        applyAlias(c, info.handle);
                    },
                    [&](const CreateAliasTextureCommand &c)
                    {
                        applyAlias(c, info.handle, info.extent);
                    },

                },
                command);
        }

        for (auto &[id, group] : newPlan.groups)
            group.requirement = m_viewToHeaps[info.handle].at(id).requirement;

        cachedAlias.aliasPlan = std::move(newPlan);
        cachedAlias.inputHash = hash;
        cachedAlias.valid = true;
    }

    std::vector<AliasCommand> ResourceManager::diffAliasPlan(const AliasPlan &newPlan, const AliasPlan &oldPlan, const Extent2D &extent)
    {

        std::vector<AliasCommand> commands;

        std::unordered_set<HeapId, HeapIdHash> targetedHeaps;
        std::unordered_set<PhysicalTextureHandle, PhysicalTextureHandleHash> targetedTextures;

        auto heapExists = [&](HeapId id)
        {
            return oldPlan.groups.contains(id);
        };

        for (auto &[id, group] : newPlan.groups)
        {
            targetedHeaps.insert(group.id);
            bool heapChanged = false;

            if (!heapExists(group.id))
            {
                commands.push_back(CreateHeapCommand{group.id, group.requirement});
                heapChanged = true;
            }
            else
            {
                auto &heap = oldPlan.groups.at(group.id);
                if (heap.requirement.size < group.requirement.size || heap.requirement.alignment < group.requirement.alignment || (heap.requirement.memoryTypeBits & group.requirement.memoryTypeBits) == 0)
                {
                    commands.push_back(DeleteHeapCommand{group.id});
                    commands.push_back(CreateHeapCommand{group.id, group.requirement});
                    heapChanged = true;
                }
            }

            for (auto &entry : group.textures)
            {
                auto *texture = m_textures.get(entry.handle);

                if (!texture || texture->info.options.lifetime != ResourceLifetimeType::Transient)
                    continue;

                targetedTextures.insert(entry.handle);

                auto currentHeap = oldPlan.textureToHeap.find(entry.handle);

                bool existed = currentHeap != oldPlan.textureToHeap.end();
                bool moved = existed && currentHeap->second != group.id;
                bool resized = existed &&
                               texture->builtExtent != extent;
                if (!existed || moved || heapChanged || resized)
                {
                    if (existed)
                        commands.push_back(DeleteAliasTextureCommand{entry.handle});
                    commands.push_back(CreateAliasTextureCommand{group.id, entry.handle});
                }
            }
        }

        for (auto it = oldPlan.textureToHeap.begin(); it != oldPlan.textureToHeap.end();)
        {

            if (targetedTextures.find(it->first) == targetedTextures.end())
            {
                commands.push_back(DeleteAliasTextureCommand{it->first});
            }
            ++it;
        }

        for (auto &[id, heap] : oldPlan.groups)
        {
            if (targetedHeaps.find(id) == targetedHeaps.end())
            {
                commands.push_back(DeleteHeapCommand{id});
            }
        }

        auto phase = [](const AliasCommand &c)
        {
            return std::visit([](const auto &x) -> int
                              {
        using T = std::decay_t<decltype(x)>;

       if constexpr (std::is_same_v<T, DeleteAliasTextureCommand>)
   { return 0;}
    else if constexpr (std::is_same_v<T, DeleteHeapCommand>)
    {  return 1;}
    else if constexpr (std::is_same_v<T, CreateHeapCommand>)
    {return 2;}
    else
    { return 3;} }, c);
        };

        std::stable_sort(commands.begin(), commands.end(), [&](const auto &a, const auto &b)
                         { return phase(a) < phase(b); });

        return commands;
    }

    void ResourceManager::applyAlias(const CreateHeapCommand &command, RenderViewHandle view)
    {
        auto &viewHeaps = m_viewToHeaps[view];

        auto heapHandle = m_device.create(command.requirement.size, command.requirement.memoryTypeBits);

        viewHeaps[command.id] = {
            .handle = heapHandle,
            .requirement = command.requirement};
    }

    void ResourceManager::applyAlias(const CreateAliasTextureCommand &command, const RenderViewHandle &view, const Extent2D &extent)
    {
        auto &viewHeaps = m_viewToHeaps[view];

        auto it = viewHeaps.find(command.id);
        assert(it != viewHeaps.end() && "alias command refers to a heap that doesn't exist");
        const auto &heap = it->second;
        auto texture = m_textures.get(command.texture);

        if (!texture)
            return;
        auto desc = texture->info.descriptor;
        desc.size.width = texture->info.size.calculateTextureWidth(extent.width);
        desc.size.height = texture->info.size.calculateTextureHeight(extent.height);

        clearResource(*texture);

        auto count = 1;

        texture->resources.create(count, [&](uint32_t frameIdx)
                                  { return m_device.createTextureFromHeap(heap.handle, desc, 0); });
        texture->builtExtent = extent;
        texture->dirty = false;
    }

    void ResourceManager::applyAlias(const DeleteAliasTextureCommand &command)
    {
        auto texture = m_textures.get(command.texture);
        if (!texture)
            return;

        clearResource(*texture);
    }

    void ResourceManager::applyAlias(const DeleteHeapCommand &command, RenderViewHandle view)
    {
        auto &viewHeaps = m_viewToHeaps[view];

        auto it = viewHeaps.find(command.id);
        assert(it != viewHeaps.end() && "alias command refers to a heap that doesn't exist");
        const auto &heap = it->second;

        m_device.destroy(heap.handle);

        viewHeaps.erase(it);
    }

    const PhysicalTextureInfo &ResourceManager::info(PhysicalTextureHandle h) const
    {
        auto *record = m_textures.get(h);
        assert(record && "info(): stale or invalid PhysicalTextureHandle");
        return record->info;
    }
    const PhysicalBufferInfo &ResourceManager::info(PhysicalBufferHandle h) const
    {
        auto *record = m_buffers.get(h);
        assert(record && "info(): stale or invalid PhysicalBufferHandle");
        return record->info;
    }

} // namespace nitro::frame

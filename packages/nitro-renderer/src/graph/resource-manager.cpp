#include "nitro-renderer/graph/resource-manager.h"
#include "nitro-rhi/rhi-constants.h"
#include "nitro-core/utils.h"
#include <algorithm>

namespace nitro::renderer
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
        assert(texture.scope == options.scope);
        assert(texture.lifetime == options.lifetime);
        assert(texture.instanceType == options.instanceType);

        bool changed =
            texture.descriptor != desc ||
            texture.size != size ||
            texture.historyCount != options.historyCount;

        if (!changed)
            return;

        texture.descriptor = desc;
        texture.size = size;
        texture.historyCount = options.historyCount;
        texture.dirty = true;

        if (texture.lifetime == ResourceLifetimeType::Persistent)
        {
            if (texture.scope == ResourceScope::Global)
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
        assert(buffer.scope == options.scope);
        assert(buffer.lifetime == options.lifetime);
        assert(buffer.instanceType == options.instanceType);

        bool changed =
            buffer.descriptor != desc ||
            buffer.size != size ||
            buffer.historyCount != options.historyCount;

        if (!changed)
            return;

        buffer.descriptor = desc;
        buffer.size = size;
        buffer.historyCount = options.historyCount;
        buffer.dirty = true;
        if (buffer.lifetime == ResourceLifetimeType::Persistent)
        {
            if (buffer.scope == ResourceScope::Global)
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
            .key = key,
            .instanceType = options.instanceType,
            .lifetime = options.lifetime,
            .scope = options.scope,
            .descriptor = desc,
            .size = size};

        physicalTexture.historyCount = options.historyCount;

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
            .key = key,
            .instanceType = options.instanceType,
            .lifetime = options.lifetime,
            .scope = options.scope,
            .descriptor = desc,
            .size = size};

        physicalBuffer.historyCount = options.historyCount;

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
        buffer.resources.forEach([=](const rhi::RHIBufferHandle &handle)
                                 { m_device.destroy(handle); });
        buffer.resources.clear();
    }
    void ResourceManager::clearResource(PhysicalTexture &texture)
    {
        texture.resources.forEach([=](const rhi::RHITextureHandle &handle)
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
        if (texture->lifetime == ResourceLifetimeType::Transient)
            return false;
        if (!needsRebuild(*texture, size))
            return false;
        clearResource(*texture);

        auto count = getResourceCount(texture->instanceType, texture->historyCount);

        texture->resources.create(count, [&](uint32_t index)
                                  {
                                      auto desc = texture->descriptor;
                                      desc.size.width = texture->size.calculateTextureWidth(size.width);
                                      desc.size.height = texture->size.calculateTextureHeight(size.height);
                                    

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

        auto count = getResourceCount(buffer->instanceType, buffer->historyCount);

        buffer->resources.create(count, [&](uint32_t index)
                                 {
                                      auto desc = buffer->descriptor;
                                      desc.size = buffer->size.calculateBufferSize(size.width,size.height);

                                      return m_device.create(desc); });

        buffer->builtExtent = size;
        buffer->dirty = false;
        return true;
    }

    bool ResourceManager::buildViewResources(const ViewBuildInfo &info)
    {
        bool historyLost = buildPersistentViewBuffers(info.handle, info.extent);
        historyLost |= buildPersistentViewTextures(info.handle, info.extent);
        rebuildTransientViewTextures(info.handle, info.extent, info.lifetimes);
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
                historyLost |= texture->instanceType == ResourceInstanceType::Temporal;
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
                historyLost |= buffer->instanceType == ResourceInstanceType::Temporal;
        }

        return historyLost;
    }
    std::vector<PhysicalTextureHandle> ResourceManager::sortLifetimeHandle(const PhysicalTextureLifetime &lifetimes)
    {
        std::vector<PhysicalTextureHandle> handles;

        for (const auto &[handle, _] : lifetimes)
            handles.push_back(handle);

        std::sort(handles.begin(), handles.end(),
                  [](auto a, auto b)
                  {
                      return PhysicalTextureHandleHash{}(a) <
                             PhysicalTextureHandleHash{}(b);
                  });
        return handles;
    }

    void ResourceManager::destroyResource(PhysicalTextureHandle handle)
    {
        auto texture = m_textures.get(handle);
        if (!texture)
            return;

        if (texture->scope == ResourceScope::External)
            texture->resources.clear();
        else
            clearResource(*texture);

        if (auto it = m_viewRelativeTexture.find(texture->key.viewHandle); it != m_viewRelativeTexture.end())
            it->second.erase(handle);

        m_pendingTextures.erase(handle);
        m_resourceKeyToTexture.erase(texture->key);
        m_textures.free(handle);
    }

    void ResourceManager::destroyResource(PhysicalBufferHandle handle)
    {
        auto buffer = m_buffers.get(handle);
        if (!buffer)
            return;

        if (buffer->scope == ResourceScope::External)
            buffer->resources.clear();
        else
            clearResource(*buffer);

        if (auto it = m_viewRelativeBuffer.find(buffer->key.viewHandle); it != m_viewRelativeBuffer.end())
            it->second.erase(handle);

        m_pendingBuffers.erase(handle);
        m_resourceKeyToBuffer.erase(buffer->key);
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
                m_device.destroy(heap);
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
        assert(t && !t->resources.empty() && t->instanceType == ResourceInstanceType::Temporal && "previous() on a non-temporal resource");
        return m_device.resolve(*t->resources.get(instanceIndex(*t, info, true)));
    }
    rhi::RHIBuffer *ResourceManager::previous(const PhysicalBufferHandle &handle, const ResourceFrameInfo &info)
    {
        auto *b = m_buffers.get(handle);
        assert(b && !b->resources.empty() && b->instanceType == ResourceInstanceType::Temporal && "previous() on a non-temporal resource");
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
            .key = key,
            .instanceType = ResourceInstanceType::Single,
            .lifetime = ResourceLifetimeType::Persistent,
            .scope = ResourceScope::External};
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
            .key = key,
            .instanceType = ResourceInstanceType::Single,
            .lifetime = ResourceLifetimeType::Persistent,
            .scope = ResourceScope::External};
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
        assert(t && t->scope == ResourceScope::External && "only imports can be re-bound");
        t->resources.clear();
        t->resources.create(1, [&](uint32_t)
                            { return rhiHandle; });
    }

    void ResourceManager::updateExternalBuffer(const PhysicalBufferHandle &handle, const rhi::RHIBufferHandle &rhiHandle)
    {
        auto *b = m_buffers.get(handle);
        assert(b && b->scope == ResourceScope::External && "only imports can be re-bound");
        b->resources.clear();
        b->resources.create(1, [&](uint32_t)
                            { return rhiHandle; });
    }

} // namespace nitro::renderer

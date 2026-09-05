#pragma once
#include "nitro-rhi/rhi.h"
#include "nitro-assets/manager.h"

namespace nitro::renderer
{
    struct GPUTextureCacheKey
    {
        assets::TextureHandle handle;
        rhi::TextureDesc::ImageFormat format;

        bool operator==(const GPUTextureCacheKey &o) const
        {
            return handle == o.handle && format == o.format;
        }
        bool operator!=(const GPUTextureCacheKey &o) const
        {
            return !(*this == o);
        }
    };

    struct GPUTextureCacheKeyHash
    {
        size_t operator()(const GPUTextureCacheKey &key) const
        {
            uint64_t h = (uint64_t(key.handle.generation) << 32) | uint64_t(key.handle.index);
            return std::hash<uint64_t>{}(h) ^ (std::hash<uint32_t>{}(uint32_t(key.format)) << 1);
        }
    };

    class GPUResourceCache
    {
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::unordered_map<GPUTextureCacheKey, rhi::RHITexture *, GPUTextureCacheKeyHash> m_textures;

    public:
        GPUResourceCache(std::shared_ptr<rhi::RHIDevice> device) : m_device(std::move(device)) {}
        ~GPUResourceCache()
        {
            for (auto &[key, texture] : m_textures)
            {
                if (texture)
                {
                    m_device->destroyTexture(texture);
                }
            }
        }
        rhi::RHITexture *get(const assets::TextureHandle &handle, assets::AssetManager &manager, rhi::TextureDesc::ImageFormat format)
        {
            GPUTextureCacheKey key{
                .handle = handle,
                .format = format};

            auto it = m_textures.find(key);

            if (it != m_textures.end())
            {
                return it->second;
            }

            auto cpuTexture = manager.getAsset(handle);
            if (!cpuTexture)
            {
                return nullptr;
            }

            rhi::TextureDesc desc;
            desc.format = format;
            desc.initialData = cpuTexture->pixels.data();
            desc.size = {static_cast<uint32_t>(cpuTexture->width), static_cast<uint32_t>(cpuTexture->height)};
            desc.usage = rhi::TextureDesc::Usage::ShaderRead;

            auto gpuTexture = m_device->createTexture(desc);

            m_textures[key] = gpuTexture;

            return gpuTexture;
        };
        void clear()
        {
            for (auto &[key, texture] : m_textures)
            {
                if (texture)
                {
                    m_device->destroyTexture(texture);
                }
            }
        }
    };
} // namespace nitro::renderer

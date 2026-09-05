#pragma once
#include "nitro-core/types/pool.h"
#include "nitro-core/uuid.h"
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

namespace nitro::assets
{

    using AssetId = UUID;
    enum class AssetType
    {
        Texture,
        Mesh,
        Material
    };

    enum class AssetState
    {
        Loading,
        Ready,
        Failed
    };

    using OptionalAssetPath = std::optional<std::filesystem::path>;

    template <typename T>
    struct AssetEntry
    {
        AssetId id = AssetId::generate();
        std::string name;
        std::unique_ptr<T> asset;
        AssetType type;
        OptionalAssetPath path{std::nullopt};
        AssetState state = AssetState::Ready;
    };

    template <typename T>
    class AssetRegistry
    {
        using AssetHandle = Handle<AssetEntry<T>>;
        ResourcePool<AssetEntry<T>> m_assets;
        std::unordered_map<std::filesystem::path, AssetHandle> m_pathToHandle;
        std::unordered_map<AssetId, AssetHandle> m_idToHandle;

    public:
        AssetHandle registerAsset(std::string name, OptionalAssetPath path, AssetType type, std::unique_ptr<T> asset)
        {
            return registerAsset(AssetId::generate(), std::move(name), path, type, std::move(asset));
        };
        AssetHandle registerAsset(AssetId id, std::string name, OptionalAssetPath path, AssetType type, std::unique_ptr<T> asset)
        {
            AssetEntry<T> entry;
            entry.asset = std::move(asset);
            entry.name = name;
            entry.path = path;
            entry.type = type;
            entry.id = id;
            auto handle = m_assets.emplace(std::move(entry));
            m_idToHandle[id] = handle;
            if (path)
                m_pathToHandle[*path] = handle;
            return handle;
        };

        T *getAsset(const AssetHandle &h)
        {
            auto entry = m_assets.get(h);

            if (!entry)
            {
                return nullptr;
            }

            return entry->asset.get();
        }
        T *getAsset(const std::filesystem::path &path)
        {
            auto it = m_pathToHandle.find(path);

            if (it == m_pathToHandle.end())
            {
                return nullptr;
            }

            return getAsset(it->second);
        }
        std::optional<AssetHandle> getAssetHandle(const std::filesystem::path &path)
        {
            auto it = m_pathToHandle.find(path);

            if (it == m_pathToHandle.end())
            {
                return std::nullopt;
            }

            return std::make_optional(it->second);
        }
        std::optional<AssetHandle> getAssetHandle(const AssetId &id)
        {
            auto it = m_idToHandle.find(id);

            if (it == m_idToHandle.end())
            {
                return std::nullopt;
            }

            return std::make_optional(it->second);
        }
        std::optional<AssetId> idOf(const AssetHandle &h)
        {
            auto *entry = m_assets.get(h);
            if (!entry)
                return std::nullopt;
            return entry->id;
        }
        void destroyAsset(const AssetHandle &h)
        {
            auto *entry = m_assets.get(h);

            if (!entry)
                return;

            m_idToHandle.erase(entry->id);
            if (entry->path)
            {
                m_pathToHandle.erase(*entry->path);
            }
            m_assets.free(h);
        };

        void forEach(std::function<void(const T *)> callback)
        {
            m_assets.forEach(
                [&](const AssetHandle &h, const AssetEntry<T> &entry)
                {
                    callback(entry.asset.get());
                }

            );
        }
        void forEachEntry(std::function<void(const AssetEntry<T> &)> callback)
        {
            m_assets.forEach(
                [&](const AssetHandle &h, const AssetEntry<T> &entry)
                {
                    callback(entry);
                }

            );
        }

        void clear()
        {
            m_assets.clear();
        }
    };

} // namespace nitro::assets

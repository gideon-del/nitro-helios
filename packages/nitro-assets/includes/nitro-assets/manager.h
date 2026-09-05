#pragma once
#include "nitro-assets/importers/texture-importer.h"
#include "nitro-assets/registry.h"
#include "mesh.h"
#include "material.h"
#include "json.hpp"
#include <fstream>

namespace nitro::assets
{

    using json = nlohmann::json;

    class AssetManager
    {
        AssetRegistry<Texture> m_textureRegistry;
        AssetRegistry<Mesh> m_meshRegistry;
        AssetRegistry<Material> m_materialRegistry;

    public:
        TextureHandle import(const std::filesystem::path &path)
        {
            return import(AssetId::generate(), path);
        };
        TextureHandle import(const AssetId &id, const std::filesystem::path &path)
        {
            auto existingHandle = m_textureRegistry.getAssetHandle(path);

            if (existingHandle)
            {
                return *existingHandle;
            }

            TextureImporter importer;

            auto texture = importer.import(path);

            return m_textureRegistry.registerAsset(id, path.filename(), OptionalAssetPath{path}, AssetType::Texture, std::make_unique<Texture>(texture));
        };

        MeshAssetHandle registerMesh(std::unique_ptr<Mesh> mesh)
        {

            return registerMesh(AssetId::generate(), std::move(mesh));
        };
        MeshAssetHandle registerMesh(const AssetId &id, std::unique_ptr<Mesh> mesh)
        {
            mesh->computeBounds();
            return m_meshRegistry.registerAsset(
                id,
                mesh->name,
                std::nullopt,
                AssetType::Mesh,
                std::move(mesh));
        };

        MaterialAssetHandle registerMaterial(std::unique_ptr<Material> material, std::string name)
        {

            return registerMaterial(AssetId::generate(), std::move(material), name);
        }
        MaterialAssetHandle registerMaterial(const AssetId &id, std::unique_ptr<Material> material, std::string name)
        {

            return m_materialRegistry.registerAsset(
                id,
                name,
                std::nullopt,
                AssetType::Material,
                std::move(material));
        }

        Texture *getAsset(const TextureHandle &handle)
        {
            return m_textureRegistry.getAsset(handle);
        }
        Mesh *getAsset(const MeshAssetHandle &handle)
        {
            return m_meshRegistry.getAsset(handle);
        }
        Material *getAsset(const MaterialAssetHandle &handle)
        {
            return m_materialRegistry.getAsset(handle);
        }

        void destroyAsset(const TextureHandle &handle)
        {
            m_textureRegistry.destroyAsset(handle);
        }
        void destroyAsset(const MeshAssetHandle &handle)
        {
            m_meshRegistry.destroyAsset(handle);
        }
        void destroyAsset(const MaterialAssetHandle &handle)
        {
            m_materialRegistry.destroyAsset(handle);
        }

        void clear()
        {
            m_textureRegistry.clear();
            m_meshRegistry.clear();
            m_materialRegistry.clear();
        };

        void serialize(json &serializer, const std::filesystem::path &outputPath)
        {

            auto binaryPath = outputPath;
            binaryPath.replace_extension(".bin");

            std::ofstream binary(binaryPath, std::ios::binary);

            if (!binary)
                throw std::runtime_error("Failed to create asset binary file");
            binary.exceptions(
                std::ios::failbit |
                std::ios::badbit);
            serializer["vertexStride"] = sizeof(geometry::Vertex);
            auto &assets = serializer["assets"];

            assets["textures"] = json::object();

            m_textureRegistry.forEachEntry(
                [&](const AssetEntry<Texture> &entry)
                {
                    auto &asset = assets["textures"][entry.id.toString()];

                    asset["name"] = entry.name;

                    if (entry.path)
                        asset["path"] = entry.path->generic_string();
                    else
                        asset["path"] = nullptr;
                });

            assets["materials"] = json::object();

            m_materialRegistry.forEachEntry(
                [&](const AssetEntry<Material> &entry)
                {
                    auto &asset = assets["materials"][entry.id.toString()];

                    asset["name"] = entry.name;

                    if (entry.path)
                        asset["path"] = entry.path->generic_string();
                    else
                        asset["path"] = nullptr;

                    asset["parameters"] = {
                        {"albedo", {entry.asset->parameters.albedo.x, entry.asset->parameters.albedo.y, entry.asset->parameters.albedo.z, entry.asset->parameters.albedo.w}},
                        {"metallic", entry.asset->parameters.metallic},
                        {"roughness", entry.asset->parameters.roughness}};

                    asset["textures"] = {
                        {"albedo", assetIdToJSON(entry.asset->textures.albedo)},
                        {"normal", assetIdToJSON(entry.asset->textures.normalMap)},
                        {"occlusion", assetIdToJSON(entry.asset->textures.occlusionMap)},
                        {"metallicRoughness", assetIdToJSON(entry.asset->textures.metallicRoughness)},
                        {"emission", assetIdToJSON(entry.asset->textures.emissive)},
                    };
                });

            assets["meshes"] = json::object();

            m_meshRegistry.forEachEntry(
                [&](const AssetEntry<Mesh> &entry)
                {
                    auto &asset = assets["meshes"][entry.id.toString()];

                    asset["name"] = entry.name;

                    if (entry.path)
                        asset["path"] = entry.path->generic_string();
                    else
                        asset["path"] = nullptr;

                    auto vertexOffset = static_cast<uint64_t>(binary.tellp());

                    binary.write(
                        reinterpret_cast<const char *>(entry.asset->vertices.data()),
                        entry.asset->vertices.size() * sizeof(geometry::Vertex));

                    auto indexOffset = static_cast<uint64_t>(binary.tellp());

                    binary.write(
                        reinterpret_cast<const char *>(entry.asset->indices.data()),
                        entry.asset->indices.size() * sizeof(uint32_t));

                    asset["vertices"] = {
                        {"offset", vertexOffset},
                        {"count", static_cast<uint64_t>(entry.asset->vertices.size())}};

                    asset["indices"] = {
                        {"offset", indexOffset},
                        {"count", static_cast<uint64_t>(entry.asset->indices.size())}};

                    asset["aabb"] = {
                        {"min", {
                                    entry.asset->aabbMin.x,
                                    entry.asset->aabbMin.y,
                                    entry.asset->aabbMin.z,
                                }},
                        {"max", {
                                    entry.asset->aabbMax.x,
                                    entry.asset->aabbMax.y,
                                    entry.asset->aabbMax.z,
                                }

                        },

                        {"radius", entry.asset->boundingSphereRadius}

                    };
                });
        };

        void load(const json &serializer, const std::filesystem::path &path)
        {

            auto vertexStride = serializer.at("vertexStride").get<uint64_t>();
            if (vertexStride !=
                sizeof(geometry::Vertex))
            {
                throw std::runtime_error(
                    "Incompatible vertex format");
            }
            auto binaryPath = path;
            binaryPath.replace_extension(".bin");

            std::ifstream binary(binaryPath, std::ios::binary);

            if (!binary)
                throw std::runtime_error(
                    "Failed to open asset binary file: " + binaryPath.string());
            binary.exceptions(
                std::ios::failbit |
                std::ios::badbit);
            const auto &assets = serializer.at("assets");

            const auto &textures = assets.at("textures");
            const auto &materials = assets.at("materials");
            const auto &meshes = assets.at("meshes");

            for (const auto &[id, asset] : textures.items())
            {

                auto textureId = AssetId::parse(id);

                if (!textureId)
                    continue;

                if (!asset.at("path").is_null())
                {
                    std::filesystem::path path =
                        asset.at("path").get<std::string>();
                    import(*textureId, path);
                }
            }

            for (const auto &[id, asset] : materials.items())
            {

                auto materialId = AssetId::parse(id);

                if (!materialId)
                    continue;

                auto material = std::make_unique<Material>();

                const auto &parameters = asset.at("parameters");
                const auto &albedo = parameters.at("albedo");
                auto name = asset.at("name").get<std::string>();
                material->parameters.albedo = {
                    albedo[0].get<float>(),
                    albedo[1].get<float>(),
                    albedo[2].get<float>(),
                    albedo[3].get<float>(),
                };
                auto texHandle = [&](const char *key) -> TextureHandle
                {
                    const auto &v = asset.at("textures").at(key);
                    return v.is_null() ? TextureHandle{} : getTextureHandle(v.get<std::string>());
                };

                material->parameters.metallic = parameters.at("metallic").get<float>();
                material->parameters.roughness = parameters.at("roughness").get<float>();

                const auto &textures = asset.at("textures");

                material->textures.albedo = texHandle("albedo");
                material->textures.normalMap = texHandle("normal");
                material->textures.metallicRoughness = texHandle("metallicRoughness");
                material->textures.occlusionMap = texHandle("occlusion");
                material->textures.emissive = texHandle("emission");

                m_materialRegistry.registerAsset(
                    *materialId,
                    name,
                    std::nullopt,
                    AssetType::Material,
                    std::move(material)

                );
            }

            for (const auto &[id, asset] : meshes.items())
            {

                auto meshId = AssetId::parse(id);

                if (!meshId)
                    continue;

                auto mesh = std::make_unique<Mesh>();
                mesh->name = asset.at("name").get<std::string>();

                // Vertices
                const auto &vertexInfo = asset.at("vertices");

                uint64_t vertexOffset =
                    vertexInfo.at("offset").get<uint64_t>();

                uint64_t vertexCount =
                    vertexInfo.at("count").get<uint64_t>();

                binary.seekg(vertexOffset, std::ios::beg);

                mesh->vertices.resize(vertexCount);

                binary.read(
                    reinterpret_cast<char *>(mesh->vertices.data()),
                    vertexStride * vertexCount);

                // Indices
                const auto &indexInfo = asset.at("indices");

                uint64_t indexOffset =
                    indexInfo.at("offset").get<uint64_t>();

                uint64_t indexCount =
                    indexInfo.at("count").get<uint64_t>();

                binary.seekg(indexOffset, std::ios::beg);

                mesh->indices.resize(indexCount);
                binary.read(
                    reinterpret_cast<char *>(mesh->indices.data()),
                    sizeof(uint32_t) * indexCount);

                const auto &aabb = asset.at("aabb");

                const auto &min = aabb.at("min");
                const auto &max = aabb.at("max");

                mesh->aabbMin = {
                    min[0].get<float>(),
                    min[1].get<float>(),
                    min[2].get<float>()};

                mesh->aabbMax = {
                    max[0].get<float>(),
                    max[1].get<float>(),
                    max[2].get<float>()};

                mesh->boundingSphereRadius =
                    aabb.at("radius").get<float>();

                m_meshRegistry.registerAsset(
                    *meshId,
                    mesh->name,
                    std::nullopt,
                    AssetType::Mesh,
                    std::move(mesh));
            }
        };

        json assetIdToJSON(const TextureHandle &handle)
        {
            auto id = m_textureRegistry.idOf(handle);

            if (!id)
                return json(nullptr);

            return json(id->toString());
        }
        json assetIdToJSON(const MaterialAssetHandle &handle)
        {
            auto id = m_materialRegistry.idOf(handle);
            std::cout << "Asset Material Id " << id.has_value() << std::endl;
            if (!id)
                return json(nullptr);

            return json(id->toString());
        }
        json assetIdToJSON(const MeshAssetHandle &handle)
        {
            auto id = m_meshRegistry.idOf(handle);

            if (!id)
                return json(nullptr);

            return json(id->toString());
        }
        std::optional<MeshAssetHandle> meshHandle(const AssetId &id)
        {
            return m_meshRegistry.getAssetHandle(id);
        }
        std::optional<MaterialAssetHandle> materialHandle(const AssetId &id)
        {
            return m_materialRegistry.getAssetHandle(id);
        }

    private:
        TextureHandle getTextureHandle(const std::string_view uuidString)
        {
            auto id = AssetId::parse(uuidString);

            if (!id)
            {
                return TextureHandle{};
            }

            auto handle = m_textureRegistry.getAssetHandle(*id);

            return handle.has_value() ? *handle : TextureHandle{};
        }
    };

} // namespace nitro::assets

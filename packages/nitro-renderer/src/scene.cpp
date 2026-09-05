#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include <nitro-renderer/scene.h>
#include <chrono>
#include <unordered_set>
#include "nitro-geometry/utils.h"
#include "json.hpp"
namespace nitro::renderer
{

    using json = nlohmann::json;

    glm::vec2 read_vec2(const tinygltf::Accessor accessor, const tinygltf::Model &model, int i)
    {
        const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

        const uint8_t *data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
        const int stride = accessor.ByteStride(bufferView);
        const float *elements = reinterpret_cast<const float *>(data + stride * i);
        glm::vec2 res;
        res.x = elements[0];
        res.y = elements[1];

        return res;
    }
    glm::vec3 read_vec3(const tinygltf::Accessor &accessor, const tinygltf::Model &model, int i)
    {
        const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

        const uint8_t *data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
        const int stride = accessor.ByteStride(bufferView);
        const float *elements = reinterpret_cast<const float *>(data + stride * i);
        glm::vec3 res;
        res.x = elements[0];
        res.y = elements[1];
        res.z = elements[2];

        return res;
    }
    glm::vec4 read_vec4(const tinygltf::Accessor &accessor, const tinygltf::Model &model, int i)
    {
        const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

        const uint8_t *data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
        const int stride = accessor.ByteStride(bufferView);
        const float *elements = reinterpret_cast<const float *>(data + stride * i);
        glm::vec4 res;
        res.x = elements[0];
        res.y = elements[1];
        res.z = elements[2];
        res.w = elements[3];

        return res;
    }

    assets::TextureHandle loadGltfTexture(std::shared_ptr<rhi::RHIDevice> device, tinygltf::Model &model, const tinygltf::TextureInfo &textureInfo, rhi::TextureDesc::ImageFormat format, std::shared_ptr<assets::AssetManager> assetManager, std::string &filePath)
    {
        const tinygltf::Texture &texture = model.textures[textureInfo.index];
        const tinygltf::Image &image = model.images[texture.source];
        std::filesystem::path baseDir = std::filesystem::path(filePath).parent_path();

        return assetManager->import(baseDir / image.uri);
    };
    assets::TextureHandle loadGltfTexture(std::shared_ptr<rhi::RHIDevice> device, tinygltf::Model &model, const tinygltf::Texture &texture, rhi::TextureDesc::ImageFormat format, std::shared_ptr<assets::AssetManager> assetManager, std::string &filePath)
    {

        const auto &image = model.images[texture.source];
        std::filesystem::path baseDir = std::filesystem::path(filePath).parent_path();

        return assetManager->import(baseDir / image.uri);
    };
    void Scene::loadGltfScene(std::string filePath, std::shared_ptr<rhi::RHIDevice> device)
    {
        tinygltf::TinyGLTF loader;
        loader.SetImagesAsIs(true);
        tinygltf::Model model;
        std::string err, warn;
        bool success = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);

        if (!err.empty())
            std::cout << "Error From Tiny GLTF: " << err << std::endl;
        if (!warn.empty())
            std::cout << "Warning From Tiny GLTF: " << warn << std::endl;
        if (!success)
            throw std::runtime_error("Failed to load tiny gltf file at " + filePath);

        tinygltf::Scene defaultScene = model.scenes[model.defaultScene];
        std::vector<GPUMaterialHandle> materialIndices;

        auto t0 = std::chrono::high_resolution_clock::now();
        for (auto &gltfMaterial : model.materials)
        {
            assets::Material material;

            if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
                material.textures.albedo = loadGltfTexture(device, model, gltfMaterial.pbrMetallicRoughness.baseColorTexture, rhi::TextureDesc::ImageFormat::ColorSRGB8, m_assetManager, filePath);

            if (gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
                material.textures.metallicRoughness = loadGltfTexture(device, model, gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture, rhi::TextureDesc::ImageFormat::ColorRGBA8, m_assetManager, filePath);

            if (gltfMaterial.normalTexture.index >= 0)
            {
                auto normalTexture = model.textures[gltfMaterial.normalTexture.index];
                material.textures.normalMap = loadGltfTexture(device, model, normalTexture, rhi::TextureDesc::ImageFormat::ColorRGBA8, m_assetManager, filePath);
            }

            if (gltfMaterial.occlusionTexture.index >= 0)
            {
                auto occlusionTexture = model.textures[gltfMaterial.occlusionTexture.index];
                material.textures.occlusionMap = loadGltfTexture(device, model, occlusionTexture, rhi::TextureDesc::ImageFormat::ColorRGBA8, m_assetManager, filePath);
            }

            if (gltfMaterial.emissiveTexture.index >= 0)
            {
                auto emissiveTexture = model.textures[gltfMaterial.emissiveTexture.index];
                material.textures.emissive = loadGltfTexture(device, model, emissiveTexture, rhi::TextureDesc::ImageFormat::ColorRGBA8, m_assetManager, filePath);
            }

            auto &pbr = gltfMaterial.pbrMetallicRoughness;
            material.parameters.albedo = glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
            material.parameters.metallic = static_cast<float>(pbr.metallicFactor);
            material.parameters.roughness = static_cast<float>(pbr.roughnessFactor);
            auto materialHandle = m_assetManager->registerMaterial(std::make_unique<assets::Material>(std::move(material)), "Material");
            materialIndices.push_back(materialManager->addMaterial(materialHandle));
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        auto ms = [](auto a, auto b)
        {
            return std::chrono::duration<double, std::milli>(b - a).count();
        };
        std::function<void(int, geometry::MeshTransformation)> walkNode;
        walkNode = [&](int nodeIdx, geometry::MeshTransformation parentTransform)
        {
            const auto &node = model.nodes[nodeIdx];

            geometry::MeshTransformation transformation = parentTransform;

            if (node.rotation.size() == 4)
                transformation.rotate(glm::qua{node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]});
            if (node.translation.size() == 3)
                transformation.translate(glm::vec3{node.translation[0], node.translation[1], node.translation[2]});
            if (node.scale.size() == 3)
                transformation.scale(glm::vec3{node.scale[0], node.scale[1], node.scale[2]});

            if (node.mesh >= 0)
            {
                tinygltf::Mesh nodeMesh = model.meshes[node.mesh];
                int primitiveIdx = 0;
                for (auto &primitive : nodeMesh.primitives)
                {

                    std::vector<geometry::Vertex> vertices;
                    std::vector<uint32_t> indices;

                    const auto &positionAccessor = model.accessors[primitive.attributes.at("POSITION")];
                    const auto &normalAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                    const auto &uvAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];

                    std::vector<glm::vec3> positions(positionAccessor.count);
                    std::vector<glm::vec3> normals(normalAccessor.count);
                    std::vector<glm::vec2> uvs(uvAccessor.count);

                    for (int i = 0; i < positionAccessor.count; i++)
                        positions[i] = read_vec3(positionAccessor, model, i);

                    for (int i = 0; i < normalAccessor.count; i++)
                        normals[i] = read_vec3(normalAccessor, model, i);

                    for (int i = 0; i < uvAccessor.count; i++)
                        uvs[i] = read_vec2(uvAccessor, model, i);

                    bool hasTangent = primitive.attributes.count("TANGENT");
                    std::vector<glm::vec4> tangents;
                    if (hasTangent)
                    {
                        tangents.resize(model.accessors[primitive.attributes.at("TANGENT")].count);

                        for (int i = 0; i < tangents.size(); i++)
                            tangents[i] = read_vec4(model.accessors[primitive.attributes.at("TANGENT")], model, i);
                        auto tangentT1 = std::chrono::high_resolution_clock::now();
                    }

                    vertices.reserve(positionAccessor.count);

                    for (int i = 0; i < positionAccessor.count; i++)
                    {
                        geometry::Vertex vertex;
                        vertex.pos = glm::vec4(positions[i], 1.0);
                        vertex.normal = normals[i];
                        vertex.uv = uvs[i];
                        if (hasTangent)
                            vertex.tangent = tangents[i];
                        vertices.push_back(vertex);
                    }

                    tinygltf::Accessor indicesAccessor = model.accessors[primitive.indices];
                    tinygltf::BufferView bufferView = model.bufferViews[indicesAccessor.bufferView];
                    tinygltf::Buffer buffer = model.buffers[bufferView.buffer];
                    const uint8_t *data = buffer.data.data() + bufferView.byteOffset + indicesAccessor.byteOffset;

                    indices.resize(indicesAccessor.count);

                    if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                    {
                        memcpy(indices.data(), data, indicesAccessor.count * sizeof(uint32_t));
                    }
                    else
                    {
                        for (size_t i = 0; i < indicesAccessor.count; i++)
                        {
                            uint32_t index;
                            switch (indicesAccessor.componentType)
                            {
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                                index = *reinterpret_cast<const uint8_t *>(data + i);
                                break;
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                                index = *reinterpret_cast<const uint16_t *>(data + i * sizeof(uint16_t));
                                break;
                            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                                index = *reinterpret_cast<const uint32_t *>(data + i * sizeof(uint32_t));
                                break;
                            default:
                                throw std::runtime_error("Unsupported index component type");
                            }
                            indices.push_back(index);
                        }
                    }

                    geometry::Mesh mesh;
                    mesh.vertices = vertices;
                    mesh.indices = indices;
                    mesh.name = nodeMesh.name;

                    if (mesh.name.empty())
                    {
                        mesh.name = "Primitive " + std::to_string(primitiveIdx) + " [mat " + std::to_string(primitive.material) + "]";
                    }
                    auto meshId = meshManager->addMesh(mesh);

                    MeshInstance instance;
                    instance.mesh = meshId;
                    instance.material = (primitive.material >= 0) ? materialIndices[primitive.material] : GPUMaterialHandle{};

                    instance.transformation = transformation;

                    addMeshInstance(meshManager->addMeshInstances(instance));

                    primitiveIdx++;
                }
            }

            for (auto childIdx : node.children)
                walkNode(childIdx, transformation);
        };

        for (auto nodeIdx : defaultScene.nodes)
            walkNode(nodeIdx, geometry::MeshTransformation{});
    }

    void Scene::addMeshInstance(const MeshInstanceHandle &handle)
    {
        auto instance = meshManager->getMeshInstance(handle);
        if (instance == nullptr)
            return;
        auto gpuMesh = meshManager->getGPUMesh(instance->mesh);
        if (!gpuMesh)
            return;
        auto mesh = m_assetManager->getAsset(gpuMesh->meshHandle);
        if (mesh == nullptr)
            return;

        instance->cells = m_grid.worldToCellRange(instance->worldAABBMin, instance->worldAABBMax);

        m_grid.addMeshInstance(handle, instance->cells);

        m_instanceIds.push_back(handle);

        markInstanceIdBuffersDirty();
    }

    OptionalMeshInstanceHandle Scene::pickMeshInstance(const geometry::Ray &ray)
    {
        MeshInstanceHandle bestInstance{};
        float bestT = std::numeric_limits<float>::max();

        m_lastPick.ray = ray;

        m_lastPick.tested.clear();
        m_lastPick.hit.clear();
        auto startPoint = ray.origin;
        auto endPoint = ray.origin + (ray.tMax * ray.dir);
        auto cellCoords = m_grid.worldToCellRange(startPoint, endPoint);

        auto instances = m_grid.getMeshInstances(cellCoords);

        for (auto &instanceHandle : instances)
        {

            auto instance = meshManager->getMeshInstance(instanceHandle);

            if (!instance)
                continue;

            m_lastPick.tested.push_back(instanceHandle);
            float tHit = 0.0f;

            if (geometry::rayAABB(ray, instance->worldAABBMin, instance->worldAABBMax, tHit) && bestT > tHit)
            {
                bestT = tHit;
                bestInstance = instanceHandle;
                m_lastPick.hit.push_back(instanceHandle);
            }
        }

        m_lastPick.best = bestInstance;
        m_selectedInstance = bestInstance;
        if (!bestInstance.isValid())
        {
            return std::nullopt;
        }

        OptionalMeshInstanceHandle result = bestInstance;

        return result;
    }

    void Scene::updateMeshInstance(const MeshInstanceHandle &handle)
    {
        auto instance = meshManager->getMeshInstance(handle);

        if (!instance)
        {
            return;
        }

        auto gpuMesh = meshManager->getGPUMesh(instance->mesh);
        if (!gpuMesh)
        {
            return;
        }
        auto mesh = m_assetManager->getAsset(gpuMesh->meshHandle);

        if (!mesh)
        {
            return;
        }

        m_grid.removeMeshInstance(handle, instance->cells);

        geometry::MeshTransformation::computeWorldAABB(
            instance->transformation.getTransform().model,
            mesh->aabbMin,
            mesh->aabbMax,
            instance->worldAABBMin,
            instance->worldAABBMax);

        instance->cells = m_grid.worldToCellRange(instance->worldAABBMin, instance->worldAABBMax);

        m_grid.addMeshInstance(handle, instance->cells);

        meshManager->markMeshInstanceAsDirty(handle);
    }

    void Scene::pushCommand(std::unique_ptr<IEditorCommand> cmd)
    {
        m_commands.push(std::move(cmd));
    }

    void Scene::reclaimMeshInstanceSlot(const MeshInstanceHandle &handle)
    {
        meshManager->reclaimInstance(handle);
    }

    void Scene::deactivateMeshInstanceSlot(const MeshInstanceHandle &handle)
    {

        auto *inst = meshManager->getMeshInstance(handle);
        if (inst)
            m_grid.removeMeshInstance(handle, inst->cells);

        if (m_selectedInstance && m_selectedInstance->index == handle.index)
            m_selectedInstance = std::nullopt;

        m_instanceIds.erase(
            std::remove(
                m_instanceIds.begin(),
                m_instanceIds.end(),
                handle),
            m_instanceIds.end());

        meshManager->deactivateMeshInstance(handle);
        markInstanceIdBuffersDirty();
    };

    void Scene::reactivateMeshInstanceSlot(MeshInstanceHandle &handle, MeshInstance instance)
    {
        auto activated = meshManager->reactivateMeshInstance(handle, instance);

        assert(activated);

        m_instanceIds.push_back(handle);
        auto inst = meshManager->getMeshInstance(handle);
        if (inst)
            m_grid.addMeshInstance(handle, inst->cells);
        markInstanceIdBuffersDirty();
    }

    void Scene::flush()
    {
        auto frameIdx = m_device->getCurrentFrameIndex();
        uint8_t bit = 1 << frameIdx;

        if (m_dirtySceneInstanceIdBufferMask & bit)
        {

            rebuildInstanceIdFrameBuffer(frameIdx);
            m_dirtySceneInstanceIdBufferMask &= ~bit;
        }
    };

    void Scene::rebuildInstanceIdFrameBuffer(uint32_t frameIdx)
    {
        auto &resource = m_sceneInstanceIdBuffers.current(frameIdx);

        std::vector<uint32_t> gpuInstanceIds;
        gpuInstanceIds.reserve(m_instanceIds.size());
        for (auto &handle : m_instanceIds)
            gpuInstanceIds.push_back(handle.index);

        const size_t needed = gpuInstanceIds.size();

        if (!resource.instanceIdBuffer || needed > resource.capacity)
        {
            m_device->waitIdle();
            if (resource.instanceIdBuffer)
                m_device->destroyBuffer(resource.instanceIdBuffer);

            if (needed > resource.capacity)
            {
                resource.capacity = std::max(needed * 2, size_t(1024));
            }

            rhi::BufferDesc desc;
            desc.size = sizeof(uint32_t) * resource.capacity;
            desc.storage = rhi::BufferDesc::StorageMode::Dynamic;
            desc.usage = rhi::BufferDesc::Usage::Storage;
            desc.initialData = nullptr;
            resource.instanceIdBuffer = m_device->createBuffer(desc);
        }

        if (needed > 0)
            resource.instanceIdBuffer->upload(gpuInstanceIds.data(),
                                              sizeof(uint32_t) * needed, 0);
    }

    void Scene::serialize(const std::filesystem::path &filepath)
    {

        try
        {
            std::filesystem::create_directories(filepath.parent_path());
            json serializer;

            serializer["version"] = 1;

            auto &scene = serializer["scene"];
            auto &instances = scene["instances"];

            instances = json::array();
            for (auto &handle : m_instanceIds)
            {
                auto meshInstance = meshManager->getMeshInstance(handle);
                if (!meshInstance)
                    continue;
                auto gpuMesh = meshManager->getGPUMesh(meshInstance->mesh);
                if (!gpuMesh)
                    continue;

                auto &instance = instances.emplace_back();

                instance["mesh"] = m_assetManager->assetIdToJSON(gpuMesh->meshHandle);
                auto position = meshInstance->transformation.baseTranslation();
                auto rotation = meshInstance->transformation.baseRotationEuler();
                auto scale = meshInstance->transformation.baseScale();

                instance["transform"] = {
                    {"position", {
                                     position.x,
                                     position.y,
                                     position.z,
                                 }},
                    {"rotation", {
                                     rotation.x,
                                     rotation.y,
                                     rotation.z,
                                 }},
                    {"scale", {
                                  scale.x,
                                  scale.y,
                                  scale.z,
                              }},
                };

                auto material = materialManager->getAssetHandle(meshInstance->material);

                if (!material.has_value())
                {
                    instance["material"] = json(nullptr);
                }
                else
                {

                    instance["material"] = m_assetManager->assetIdToJSON(*material);
                }
            }
            m_assetManager->serialize(serializer, filepath);
            std::ofstream file(filepath);

            if (!file)
                throw std::runtime_error(
                    "Failed to create scene file: " + filepath.string());

            file << serializer.dump(4);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    };

    void Scene::clear()
    {
        m_grid.clear();
        m_instanceIds.clear();
        m_selectedInstance = std::nullopt;
        m_lastPick = PickDebug{};
        m_assetManager->clear();
        m_commands.clear();
        materialManager->clear();
        meshManager->clear();
    };

    bool Scene::load(const std::filesystem::path &filepath)
    {

        try
        {

            std::ifstream file(filepath);

            if (!file)
                return false;

            json serializer;

            file >> serializer;

            m_device->waitIdle();
            clear();

            m_assetManager->load(serializer, filepath);

            const auto &scene = serializer.at("scene");
            const auto &instances = scene.at("instances");

            for (auto &inst : instances)
            {
                auto meshId = AssetId::parse(inst.at("mesh").get<std::string>());
                if (!meshId)
                    continue;
                auto meshAsset = m_assetManager->meshHandle(*meshId);
                if (!meshAsset)
                    continue;
                auto gpuMesh = meshManager->addMeshFromAsset(*meshAsset);

                GPUMaterialHandle gpuMat{};

                if (!inst.at("material").is_null())
                {
                    auto matId = AssetId::parse(inst.at("material").get<std::string>());
                    if (matId)
                    {
                        auto matAsset = m_assetManager->materialHandle(*matId);
                        std::cout << "Mat Asset handle " << matAsset.has_value() << std::endl;

                        if (matAsset)
                        {
                            std::cout << "Mat Asset: " << matAsset.value().index << std::endl;
                            gpuMat = materialManager->addMaterial(*matAsset);
                        }
                    }
                }
                auto &t = inst.at("transform");
                geometry::MeshTransformation xf;
                auto p = t.at("position");
                xf.setTranslation({p[0], p[1], p[2]});
                auto r = t.at("rotation");
                xf.setRotationEuler({r[0], r[1], r[2]});
                auto s = t.at("scale");
                xf.setScale({s[0], s[1], s[2]});

                MeshInstance mi;
                mi.mesh = gpuMesh;
                mi.material = gpuMat;
                mi.transformation = xf;
                addMeshInstance(meshManager->addMeshInstances(mi));
            }

            meshManager->buildMegaBuffers();
            materialManager->buildMegaMaterialBuffer();
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';

            return false;
        }
    }

} // namespace nitro::renderer

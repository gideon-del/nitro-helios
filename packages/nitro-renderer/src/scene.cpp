#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "../vendor/tiny_gltf.h"
#include <nitro-renderer/scene.h>
#include <chrono>

namespace nitro::renderer
{

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

    rhi::RHITexture *loadGltfTexture(std::shared_ptr<rhi::RHIDevice> device, tinygltf::Model &model, const tinygltf::TextureInfo &textureInfo, rhi::TextureDesc::ImageFormat format)
    {
        const tinygltf::Texture &texture = model.textures[textureInfo.index];
        const tinygltf::Image &image = model.images[texture.source];
        rhi::TextureDesc textureDesc;
        textureDesc.size = {static_cast<uint32_t>(image.width), static_cast<uint32_t>(image.height)};
        textureDesc.format = format;
        textureDesc.initialData = image.image.data();
        textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead;

        return device->createTexture(textureDesc);
    };
    rhi::RHITexture *loadGltfTexture(std::shared_ptr<rhi::RHIDevice> device, tinygltf::Model &model, const tinygltf::Texture &texture, rhi::TextureDesc::ImageFormat format)
    {

        const auto &image = model.images[texture.source];
        rhi::TextureDesc textureDesc;
        textureDesc.size = {static_cast<uint32_t>(image.width), static_cast<uint32_t>(image.height)};
        textureDesc.format = format;
        textureDesc.initialData = image.image.data();
        textureDesc.usage = rhi::TextureDesc::Usage::ShaderRead;

        return device->createTexture(textureDesc);
    };
    void Scene::loadGltfScene(std::string filePath, std::shared_ptr<rhi::RHIDevice> device)
    {
        tinygltf::TinyGLTF loader;
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
        std::vector<uint32_t> materialIndices;

        auto t0 = std::chrono::high_resolution_clock::now();
        for (auto &gltfMaterial : model.materials)
        {
            MaterialDesc desc;

            if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
                desc.textures.albedo = loadGltfTexture(device, model, gltfMaterial.pbrMetallicRoughness.baseColorTexture, rhi::TextureDesc::ImageFormat::ColorSRGB8);

            if (gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
                desc.textures.metallicRoughness = loadGltfTexture(device, model, gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture, rhi::TextureDesc::ImageFormat::ColorRGBA8);

            if (gltfMaterial.normalTexture.index >= 0)
            {
                auto normalTexture = model.textures[gltfMaterial.normalTexture.index];
                desc.textures.normalMap = loadGltfTexture(device, model, normalTexture, rhi::TextureDesc::ImageFormat::ColorRGBA8);
            }

            if (gltfMaterial.occlusionTexture.index >= 0)
            {
                auto occlusionTexture = model.textures[gltfMaterial.occlusionTexture.index];
                desc.textures.occlusionMap = loadGltfTexture(device, model, occlusionTexture, rhi::TextureDesc::ImageFormat::ColorRGBA8);
            }

            if (gltfMaterial.emissiveTexture.index >= 0)
            {
                auto emissiveTexture = model.textures[gltfMaterial.emissiveTexture.index];
                desc.textures.emissive = loadGltfTexture(device, model, emissiveTexture, rhi::TextureDesc::ImageFormat::ColorRGBA8);
            }

            auto &pbr = gltfMaterial.pbrMetallicRoughness;
            desc.parameters.albedo = glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
            desc.parameters.metallic = static_cast<float>(pbr.metallicFactor);
            desc.parameters.roughness = static_cast<float>(pbr.roughnessFactor);

            materialIndices.push_back(materialManager->addMaterial(desc));
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
                    auto positionT0 = std::chrono::high_resolution_clock::now();
                    for (int i = 0; i < positionAccessor.count; i++)
                        positions[i] = read_vec3(positionAccessor, model, i);
                    auto positionT1 = std::chrono::high_resolution_clock::now();

                    std::cout << "Postion Load : " << ms(positionT0, positionT1) << " ms\n";

                    auto normalT0 = std::chrono::high_resolution_clock::now();
                    for (int i = 0; i < normalAccessor.count; i++)
                        normals[i] = read_vec3(normalAccessor, model, i);
                    auto normalT1 = std::chrono::high_resolution_clock::now();
                    std::cout << "Normal Load : " << ms(normalT0, normalT1) << " ms\n";

                    auto uvT0 = std::chrono::high_resolution_clock::now();
                    for (int i = 0; i < uvAccessor.count; i++)
                        uvs[i] = read_vec2(uvAccessor, model, i);
                    auto uvT1 = std::chrono::high_resolution_clock::now();

                    std::cout << "UV Load : " << ms(uvT0, uvT1) << " ms\n";

                    bool hasTangent = primitive.attributes.count("TANGENT");
                    std::vector<glm::vec4> tangents;
                    if (hasTangent)
                    {
                        tangents.resize(model.accessors[primitive.attributes.at("TANGENT")].count);
                        auto tangentT0 = std::chrono::high_resolution_clock::now();
                        for (int i = 0; i < tangents.size(); i++)
                            tangents[i] = read_vec4(model.accessors[primitive.attributes.at("TANGENT")], model, i);
                        auto tangentT1 = std::chrono::high_resolution_clock::now();
                        std::cout << "Tangent Load : " << ms(tangentT0, tangentT1) << " ms\n";
                    }

                    vertices.reserve(positionAccessor.count);
                    auto vertexT0 = std::chrono::high_resolution_clock::now();
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
                    auto vertexT1 = std::chrono::high_resolution_clock::now();
                    std::cout << "Vertex Copy : " << ms(vertexT0, vertexT1) << " ms\n";
                    tinygltf::Accessor indicesAccessor = model.accessors[primitive.indices];
                    tinygltf::BufferView bufferView = model.bufferViews[indicesAccessor.bufferView];
                    tinygltf::Buffer buffer = model.buffers[bufferView.buffer];
                    const uint8_t *data = buffer.data.data() + bufferView.byteOffset + indicesAccessor.byteOffset;

                    indices.resize(indicesAccessor.count);
                    auto indexT0 = std::chrono::high_resolution_clock::now();

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
                    auto indexT1 = std::chrono::high_resolution_clock::now();
                    std::cout << "Index Copy : " << ms(indexT0, indexT1) << " ms\n";

                    geometry::Mesh mesh;
                    mesh.vertices = vertices;
                    mesh.indices = indices;
                    auto meshT0 = std::chrono::high_resolution_clock::now();

                    uint32_t meshId = meshManager->addMesh(mesh);
                    auto meshT1 = std::chrono::high_resolution_clock::now();

                    std::cout << "Mesh Id : " << meshId << " Load time: " << ms(meshT0, meshT1) << " ms\n";
                    std::cout << "Vertex Count: " << vertices.size() << "\n";

                    MeshInstance instance;
                    instance.meshId = meshId;
                    instance.materialId = (primitive.material >= 0) ? materialIndices[primitive.material] : INVALID_MATERIAL_INDEX;

                    auto transform = transformation.getTransform();
                    instance.modelTransform = transform.model;
                    instance.normalTransform = transform.normalMatrix;

                    instanceIds.push_back(meshManager->addMeshInstances(instance));
                }
            }

            for (auto childIdx : node.children)
                walkNode(childIdx, transformation);
        };

        auto t2 = std::chrono::high_resolution_clock::now();
        for (auto nodeIdx : defaultScene.nodes)
            walkNode(nodeIdx, geometry::MeshTransformation{});

        auto t3 = std::chrono::high_resolution_clock::now();

        std::cout << "Material setup : " << ms(t0, t1) << " ms\n";
        std::cout << "Mesh parsing   : " << ms(t2, t3) << " ms\n";
    }
} // namespace nitro::renderer

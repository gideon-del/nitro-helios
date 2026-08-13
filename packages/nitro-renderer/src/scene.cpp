#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "../vendor/tiny_gltf.h"
#include <nitro-renderer/scene.h>

namespace nitro::renderer
{

    glm::vec2 read_vec2(const tinygltf::Accessor accessor, const tinygltf::Model &model, int i)
    {
        tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
        tinygltf::Buffer buffer = model.buffers[bufferView.buffer];

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

                    for (int i = 0; i < positionAccessor.count; i++)
                    {
                        geometry::Vertex vertex;
                        vertex.pos = read_vec3(positionAccessor, model, i);
                        vertex.normal = read_vec3(normalAccessor, model, i);
                        vertex.uv = read_vec2(uvAccessor, model, i);
                        if (primitive.attributes.count("TANGENT"))
                            vertex.tangent = read_vec4(model.accessors[primitive.attributes.at("TANGENT")], model, i);
                        vertices.push_back(vertex);
                    }

                    tinygltf::Accessor indicesAccessor = model.accessors[primitive.indices];
                    tinygltf::BufferView bufferView = model.bufferViews[indicesAccessor.bufferView];
                    tinygltf::Buffer buffer = model.buffers[bufferView.buffer];
                    const uint8_t *data = buffer.data.data() + bufferView.byteOffset + indicesAccessor.byteOffset;

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

                    geometry::Mesh mesh;
                    mesh.vertices = vertices;
                    mesh.indices = indices;

                    uint32_t meshId = meshManager->addMesh(mesh);

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

        for (auto nodeIdx : defaultScene.nodes)
            walkNode(nodeIdx, geometry::MeshTransformation{});
    }
} // namespace nitro::renderer

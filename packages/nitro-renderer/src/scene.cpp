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
    std::vector<RenderObject> Scene::loadGltfScene(std::string filePath, std::shared_ptr<rhi::RHIDevice> device)
    {
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string err;
        std::string warn;
        bool success = loader.LoadASCIIFromFile(
            &model,
            &err,
            &warn,
            filePath);

        if (!err.empty())
        {
            std::cout << "Error From Tiny GLTF: " << err << std::endl;
        }
        if (!warn.empty())
        {
            std::cout << "Warning From Tiny GLTF: " << warn << std::endl;
        }

        if (!success)
        {
            throw std::runtime_error("Failed to load tiny gltf file at " + filePath);
        }
        tinygltf::Scene defaultScene = model.scenes[model.defaultScene];
        std::vector<RenderObject> renderObjects;
        for (auto nodeIdx : defaultScene.nodes)
        {
            tinygltf::Node node = model.nodes[nodeIdx];
            geometry::MeshTransformation transformation;

            if (node.rotation.size() == 4)
            {
                auto rotation = glm::qua{node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]};
                transformation.rotate(rotation);
            }

            if (node.translation.size() == 3)
            {
                transformation.translate(glm::vec3{node.translation[0], node.translation[1], node.translation[2]});
            }

            if (node.scale.size() == 3)
            {
                transformation.scale(glm::vec3{node.scale[0], node.scale[1], node.scale[2]});
            }

            tinygltf::Mesh nodeMesh = model.meshes[node.mesh];
            for (auto &primitive : nodeMesh.primitives)
            {
                std::cout << "Loading Primitive " << std::endl;
                std::vector<geometry::Vertex> vertices;
                std::vector<uint32_t> indices;
                tinygltf::Accessor positionAccessor = model.accessors[primitive.attributes.at("POSITION")];
                tinygltf::Accessor normalAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                tinygltf::Accessor uvAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];

                for (int i = 0; i < positionAccessor.count; i++)
                {
                    geometry::Vertex vertex;

                    vertex.pos = read_vec3(positionAccessor, model, i);
                    vertex.normal = read_vec3(normalAccessor, model, i);
                    vertex.uv = read_vec2(uvAccessor, model, i);
                    if (primitive.attributes.count("TANGENT"))
                    {
                        tinygltf::Accessor tangentAccessor = model.accessors[primitive.attributes.at("TANGENT")];
                        vertex.tangent = read_vec4(tangentAccessor, model, i);
                    }

                    vertices.push_back(vertex);
                };

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
                        index = *reinterpret_cast<const uint8_t *>(data + i * sizeof(uint8_t));
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

                std::shared_ptr<MeshRenderer> renderer = std::make_shared<MeshRenderer>(mesh, device);
                auto material = std::make_shared<Material>();
                material->roughnessFactor = 1.0;
                material->metallicFactor = 1.0;
                material->baseColorFactor = {1.0, 1.0, 1.0, 1.0};
                renderObjects.push_back(RenderObject(renderer, transformation, material));

                std::cout << "Done with Loading Primitive " << std::endl;
            };
        }

        return renderObjects;
    }
} // namespace nitro::renderer

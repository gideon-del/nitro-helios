#pragma once
#include <nitro-geometry/mesh.h>
#include <nitro-geometry/mesh-transformation.h>
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{
    struct MeshDescriptor
    {
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
        glm::vec3 aabbMin = glm::vec3(0);
        float _pad1;
        glm::vec3 aabbMax = glm::vec3(0);
    };
    struct MeshInstance
    {
        uint32_t meshId = 0xFFFFFFFFu;
        uint32_t materialId = 0xFFFFFFFFu;
        glm::mat4 modelTransform;
        glm::mat4 normalTransform;
    };

    struct MeshInfo
    {
        uint32_t vertexOffset = 0;
        uint32_t indexOffset = 0;
        geometry::Mesh mesh;
    };
    class MeshManager
    {
    public:
        MeshManager(std::shared_ptr<rhi::RHIDevice> device);
        ~MeshManager();
        uint32_t addMesh(geometry::Mesh mesh);
        uint32_t addMeshInstances(MeshInstance &instance);
        void buildMegaBuffers();

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::vector<MeshInfo> m_meshes;
        std::vector<MeshInstance> m_instances;
        rhi::RHIBuffer *m_vertexMegaBuffer = nullptr;
        rhi::RHIBuffer *m_indexMegaBuffer = nullptr;
        rhi::RHIBuffer *m_meshDescriptorBuffer = nullptr;
        rhi::RHIBuffer *m_meshInstanceBuffer = nullptr;
    };
} // namespace nitro::renderer

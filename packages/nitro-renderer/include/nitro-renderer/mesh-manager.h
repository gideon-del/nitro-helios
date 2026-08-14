#pragma once
#include <nitro-geometry/mesh.h>
#include <nitro-geometry/mesh-transformation.h>
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>

namespace nitro::renderer
{

    static constexpr uint32_t INVALID_MATERIAL_INDEX = 0xFFFFFFFFu;
    struct MeshDescriptor
    {
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
        glm::vec3 aabbMin = glm::vec3(0);
        float _pad1;
        glm::vec3 aabbMax = glm::vec3(0);
        float _pad2[2];
    };
    struct MeshInstance
    {
        uint32_t meshId;
        uint32_t materialId = INVALID_MATERIAL_INDEX;
        uint32_t _pad0[2] = {0, 0};
        glm::mat4 modelTransform = glm::mat4(1.0f);
        glm::mat4 normalTransform = glm::mat4(1.0f);
    };

    struct MeshInfo
    {
        uint32_t vertexOffset = 0;
        uint32_t indexOffset = 0;
        geometry::Mesh mesh;
        glm::vec3 aabbMin = glm::vec3(0.0f);
        glm::vec3 aabbMax = glm::vec3(0.0f);
    };
    class MeshManager
    {
    public:
        MeshManager(std::shared_ptr<rhi::RHIDevice> device);
        ~MeshManager();
        uint32_t addMesh(geometry::Mesh mesh);
        uint32_t addMeshInstances(MeshInstance &instance);
        rhi::RHIBuffer *getVertexMegaBuffer() { return m_vertexMegaBuffer; }
        rhi::RHIBuffer *getIndexMegaBuffer() { return m_indexMegaBuffer; }
        rhi::RHIBuffer *instanceBuffer() { return m_meshInstanceBuffer; }
        rhi::RHIBuffer *descriptorBuffer() { return m_meshDescriptorBuffer; }
        uint32_t instanceCount() { return static_cast<uint32_t>(m_instances.size()); }
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

#pragma once
#include <nitro-geometry/mesh.h>
#include <nitro-geometry/mesh-transformation.h>
#include <nitro-rhi/rhi.h>
#include <glm/glm.hpp>
#include "spatial-grid-coord.h"
#include "per-frame.h"
#include "handles.h"
#include "nitro-core/types/pool.h"

namespace nitro::renderer
{

    static constexpr uint32_t INVALID_MATERIAL_INDEX = 0xFFFFFFFFu;
    static constexpr uint32_t MESH_LOD_COUT = 4;
    struct MeshLOD
    {
        std::vector<uint32_t> indices;
        size_t indexOffset;
        float screenThreshold;
        float _pad;
    };

    struct alignas(16) MeshLODDescriptor
    {
        uint32_t indexOffset;
        uint32_t indexCount;
        float screenThreshold;
        float _pad;
    };
    struct alignas(16) MeshDescriptor
    {
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
        float _pad1;
        glm::vec3 aabbMin = glm::vec3(0);
        float boundingSphereRadius = 0.0f;
        glm::vec3 aabbMax = glm::vec3(0);
        float _pad2;
        MeshLODDescriptor lod[MESH_LOD_COUT];
    };

    struct alignas(16) MeshInstanceDesc
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
        uint32_t indexCount = 0;
        MeshLOD meshLod[MESH_LOD_COUT];
        geometry::Mesh mesh;
        glm::vec3 aabbMin = glm::vec3(0.0f);
        glm::vec3 aabbMax = glm::vec3(0.0f);
        float boundingSphereRadius = 0.0f;
    };
    static_assert(sizeof(MeshLODDescriptor) == 16);
    static_assert(alignof(MeshLODDescriptor) == 16);

    static_assert(sizeof(MeshDescriptor) == 112);
    static_assert(alignof(MeshDescriptor) == 16);

    struct MeshManagerResource
    {
        rhi::RHIBuffer *instanceBuffer = nullptr;
        rhi::RHIBuffer *descriptorBuffer = nullptr;
        rhi::RHIBuffer *vertexBuffer = nullptr;
        rhi::RHIBuffer *indexBuffer = nullptr;

        size_t instanceCapacity = 1024;
        size_t meshCapacity = 1024;
    };

    class MeshManager
    {
    public:
        MeshManager(std::shared_ptr<rhi::RHIDevice> device);
        ~MeshManager();
        MeshHandle addMesh(geometry::Mesh mesh);
        MeshInstanceHandle addMeshInstances(MeshInstance &instance);
        void reclaimInstance(const MeshInstanceHandle &handle);
        void deactivateMeshInstance(const MeshInstanceHandle &handle);
        bool reactivateMeshInstance(MeshInstanceHandle &handle, MeshInstance &instance);
        rhi::RHIBuffer *getVertexMegaBuffer() { return m_resources.current(m_device->getCurrentFrameIndex()).vertexBuffer; }
        rhi::RHIBuffer *getIndexMegaBuffer() { return m_resources.current(m_device->getCurrentFrameIndex()).indexBuffer; }
        rhi::RHIBuffer *instanceBuffer() { return m_resources.current(m_device->getCurrentFrameIndex()).instanceBuffer; }
        rhi::RHIBuffer *descriptorBuffer() { return m_resources.current(m_device->getCurrentFrameIndex()).descriptorBuffer; }
        uint32_t instanceCount() { return static_cast<uint32_t>(m_instances.size()); }
        void buildMegaBuffers();
        MeshInfo *getMesh(const MeshHandle &handle);
        MeshInstance *getMeshInstance(const MeshInstanceHandle handle);
        void markMeshInstanceAsDirty(const MeshInstanceHandle &handle);
        void flusDirtyMeshInstances();
        size_t poolCapacity() const { m_instances.size(); }

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::vector<MeshInfo> m_meshes;
        ResourcePool<MeshInstance> m_instances;
        PerFrame<MeshManagerResource> m_resources;
        uint8_t m_dirtyInstanceBufferMask = 0;
        uint8_t m_dirtyDescriptorBufferMask = 0;

        std::vector<MeshInstanceHandle> m_dirtyInstances;
        void updateMeshInstanceBuffer(MeshInstance &instance, uint32_t id, uint32_t frameIdx);

        MeshInstanceDesc createInstanceDesc(MeshInstance &instance);
        void rebuildFrameInstanceBuffer(uint32_t frameIdx);
        void buildMeshDescriptorBuffer(uint32_t frameIdx);
        void buildMegaVertexBuffer(uint32_t frameidx);
        void buildMegaIndexBuffer(uint32_t frameidx);
        void markMeshDescriptorDirty()
        {
            m_dirtyDescriptorBufferMask = (1 << g_MAX_FRAMES_IN_FLIGHT) - 1;
        }
        void markMeshInstanceBufferDirty()
        {
            m_dirtyInstanceBufferMask = (1 << g_MAX_FRAMES_IN_FLIGHT) - 1;
        }
    };
} // namespace nitro::renderer

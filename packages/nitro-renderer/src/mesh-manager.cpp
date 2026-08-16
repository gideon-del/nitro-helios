#include "nitro-renderer/mesh-manager.h"
#include "meshoptimizer.h"

namespace nitro::renderer
{
    MeshLOD generateMeshLOD(geometry::Mesh &mesh, float ratio)
    {

        MeshLOD lod;

        size_t targetIndexCount =
            static_cast<size_t>(
                mesh.indices.size() * ratio);

        targetIndexCount =
            (targetIndexCount / 3) * 3;

        lod.indices.resize(mesh.indices.size());

        size_t indexCount = meshopt_simplify(
            lod.indices.data(),
            mesh.indices.data(),
            mesh.indices.size(),
            reinterpret_cast<const float *>(
                mesh.vertices.data()),
            mesh.vertices.size(),
            sizeof(geometry::Vertex),

            targetIndexCount,
            0.01f,
            0,
            nullptr);

        lod.indices.resize(indexCount);

        return lod;
    };

    MeshManager::MeshManager(std::shared_ptr<rhi::RHIDevice> device) : m_device(device) {

                                                                       };
    MeshManager::~MeshManager()
    {
        if (m_vertexMegaBuffer)
        {
            m_device->destroyBuffer(m_vertexMegaBuffer);
        }
        if (m_indexMegaBuffer)
        {
            m_device->destroyBuffer(m_indexMegaBuffer);
        }
        if (m_meshDescriptorBuffer)
        {
            m_device->destroyBuffer(m_meshDescriptorBuffer);
        }
        if (m_meshInstanceBuffer)
        {
            m_device->destroyBuffer(m_meshInstanceBuffer);
        }
    }

    uint32_t MeshManager::addMesh(geometry::Mesh mesh)
    {
        uint32_t id = static_cast<uint32_t>(m_meshes.size());

        MeshInfo info;
        info.mesh = mesh;

        if (id != 0)
        {
            auto &prev = m_meshes.back();
            info.vertexOffset = prev.vertexOffset + prev.mesh.vertices.size();
            info.indexOffset = prev.indexOffset + prev.indexCount;
        }

        glm::vec3 aabbMin(FLT_MAX), aabbMax(-FLT_MAX);
        for (auto &v : mesh.vertices)
        {
            aabbMin = glm::min(aabbMin, glm::vec3(v.pos));
            aabbMax = glm::max(aabbMax, glm::vec3(v.pos));
        }
        info.aabbMax = aabbMax;
        info.aabbMin = aabbMin;

        info.boundingSphereRadius = glm::length(info.aabbMax - info.aabbMin) * 0.5f;

        constexpr float LOD_RATIOS[MESH_LOD_COUT] =
            {
                1.0f,
                0.5f,
                0.25f,
                0.125f};

        constexpr float LOD_SCREEN_THRESHOLDS[MESH_LOD_COUT] = {
            1.0f,
            0.15f,
            0.07f,
            0.03f};
        MeshLOD lod0;
        lod0.indices = mesh.indices;
        lod0.indexOffset = info.indexOffset;
        lod0.screenThreshold = LOD_SCREEN_THRESHOLDS[0];
        info.indexCount = mesh.indices.size();
        std::cout << "Mesh ID " << id << std::endl;
        info.meshLod[0] = std::move(lod0);

        for (int i = 1; i < MESH_LOD_COUT; i++)
        {
            info.meshLod[i] = generateMeshLOD(mesh, LOD_RATIOS[i]);
            info.meshLod[i].indexOffset = info.meshLod[i - 1].indexOffset + info.meshLod[i - 1].indices.size();
            info.indexCount += info.meshLod[i].indices.size();
            info.meshLod[i].screenThreshold = LOD_SCREEN_THRESHOLDS[i];

            std::cout << "Mesh LOD " << i << " Index Offset " << info.meshLod[i].indexOffset << std::endl;
        };
        m_meshes.push_back(info);
        return id;
    }

    uint32_t MeshManager::addMeshInstances(MeshInstance &instance)
    {
        uint32_t id = static_cast<uint32_t>(m_instances.size());
        m_instances.push_back(std::move(instance));
        return id;
    }

    void MeshManager::buildMegaBuffers()
    {

        if (m_meshes.size() == 0)
            return;

        auto &lastMesh = m_meshes.back();
        size_t totalVertices = lastMesh.vertexOffset + lastMesh.mesh.vertices.size();
        size_t totalIndices = lastMesh.indexOffset + lastMesh.indexCount;

        std::vector<geometry::Vertex> vertices(totalVertices);
        std::vector<uint32_t> indices(totalIndices);

        for (auto &info : m_meshes)
        {
            memcpy(vertices.data() + info.vertexOffset, info.mesh.vertices.data(), sizeof(geometry::Vertex) * info.mesh.vertices.size());

            for (int i = 0; i < MESH_LOD_COUT; i++)
            {
                memcpy(indices.data() + info.meshLod[i].indexOffset, info.meshLod[i].indices.data(), sizeof(uint32_t) * info.meshLod[i].indices.size());
            }
        }

        rhi::BufferDesc vertexDesc;
        vertexDesc.initialData = vertices.data();
        vertexDesc.size = sizeof(geometry::Vertex) * totalVertices;
        vertexDesc.storage = rhi::BufferDesc::StorageMode::GPU;
        vertexDesc.usage = rhi::BufferDesc::Usage::Vertex;

        if (m_vertexMegaBuffer)
        {
            m_device->destroyBuffer(m_vertexMegaBuffer);
        }

        m_vertexMegaBuffer = m_device->createBuffer(vertexDesc);

        rhi::BufferDesc indexDesc;
        indexDesc.initialData = indices.data();
        indexDesc.size = sizeof(uint32_t) * totalIndices;
        indexDesc.storage = rhi::BufferDesc::StorageMode::GPU;
        indexDesc.usage = rhi::BufferDesc::Usage::Index;

        if (m_indexMegaBuffer)
        {
            m_device->destroyBuffer(m_indexMegaBuffer);
        }

        m_indexMegaBuffer = m_device->createBuffer(indexDesc);

        std::vector<MeshDescriptor> descriptors;

        for (auto &meshInfo : m_meshes)
        {
            MeshDescriptor descriptor{};
            descriptor.indexOffset = meshInfo.indexOffset;
            descriptor.indexCount = static_cast<uint32_t>(meshInfo.mesh.indices.size());
            descriptor.vertexOffset = meshInfo.vertexOffset;
            descriptor.aabbMax = meshInfo.aabbMax;
            descriptor.aabbMin = meshInfo.aabbMin;
            descriptor.boundingSphereRadius = meshInfo.boundingSphereRadius;

            for (int i = 0; i < MESH_LOD_COUT; i++)
            {
                MeshLODDescriptor lodDescriptor{};
                lodDescriptor.indexCount = static_cast<uint32_t>(meshInfo.meshLod[i].indices.size());
                lodDescriptor.indexOffset = static_cast<uint32_t>(meshInfo.meshLod[i].indexOffset);
                lodDescriptor.screenThreshold = meshInfo.meshLod[i].screenThreshold;

                descriptor.lod[i] = std::move(lodDescriptor);
            }

            descriptors.push_back(std::move(descriptor));
        }

        rhi::BufferDesc descriptorDesc;
        descriptorDesc.initialData = descriptors.data();
        descriptorDesc.size = sizeof(MeshDescriptor) * descriptors.size();
        descriptorDesc.storage = rhi::BufferDesc::StorageMode::GPU;
        descriptorDesc.usage = rhi::BufferDesc::Usage::Storage;

        if (m_meshDescriptorBuffer)
        {
            m_device->destroyBuffer(m_meshDescriptorBuffer);
        }

        m_meshDescriptorBuffer = m_device->createBuffer(descriptorDesc);

        rhi::BufferDesc instanceDesc;
        instanceDesc.initialData = m_instances.data();
        instanceDesc.size = sizeof(MeshInstance) * m_instances.size();
        instanceDesc.storage = rhi::BufferDesc::StorageMode::GPU;
        instanceDesc.usage = rhi::BufferDesc::Usage::Storage;

        if (m_meshInstanceBuffer)
            m_device->destroyBuffer(m_meshInstanceBuffer);
        m_meshInstanceBuffer = m_device->createBuffer(instanceDesc);
    }
} // namespace nitro::renderer

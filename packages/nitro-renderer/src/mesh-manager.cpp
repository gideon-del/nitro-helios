#include "nitro-renderer/mesh-manager.h"

namespace nitro::renderer
{
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
            info.indexOffset = prev.indexOffset + prev.mesh.indices.size();
        }
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
        size_t totalIndices = lastMesh.indexOffset + lastMesh.mesh.indices.size();

        std::vector<geometry::Vertex> vertices(totalVertices);
        std::vector<uint32_t> indices(totalIndices);

        for (auto &info : m_meshes)
        {
            memcpy(vertices.data() + info.vertexOffset, info.mesh.vertices.data(), sizeof(geometry::Vertex) * info.mesh.vertices.size());
            memcpy(indices.data() + info.indexOffset, info.mesh.indices.data(), sizeof(uint32_t) * info.mesh.indices.size());
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
            MeshDescriptor descriptor;
            descriptor.indexOffset = meshInfo.indexOffset;
            descriptor.indexCount = static_cast<uint32_t>(meshInfo.mesh.indices.size());
            descriptor.vertexOffset = meshInfo.vertexOffset;

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

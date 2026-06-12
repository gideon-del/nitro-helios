#include <nitro-renderer/mesh-renderer.h>

namespace nitro::renderer
{
    MeshRenderer::MeshRenderer(const geometry::Mesh &mesh, std::shared_ptr<rhi::RHIDevice> device) : m_device(device)
    {
        rhi::BufferDesc vertexDesc;
        vertexDesc.initialData = mesh.vertices.data();
        vertexDesc.size = sizeof(geometry::Vertex) * mesh.vertices.size();
        vertexDesc.storage = rhi::BufferDesc::StorageMode::GPU;
        vertexDesc.usage = rhi::BufferDesc::Usage::Vertex;

        m_vertexBuffer = m_device->createBuffer(vertexDesc);
        rhi::BufferDesc indexDesc;
        indexDesc.initialData = mesh.indices.data();
        indexDesc.size = sizeof(uint32_t) * mesh.indices.size();
        indexDesc.storage = rhi::BufferDesc::StorageMode::GPU;
        indexDesc.usage = rhi::BufferDesc::Usage::Index;

        m_indexBuffer = m_device->createBuffer(indexDesc);

        m_indexCount = static_cast<uint32_t>(mesh.indices.size());
        m_vertexCount = static_cast<uint32_t>(mesh.vertices.size());
    }

    MeshRenderer::~MeshRenderer()
    {
        m_device->destroyBuffer(m_vertexBuffer);
        m_device->destroyBuffer(m_indexBuffer);
    }

    void MeshRenderer::draw(rhi::RHICommandBuffer *cmd)
    {
        cmd->bindVertexBuffer(m_vertexBuffer);
        cmd->updateVertexCount(m_vertexCount);
        cmd->bindIndexBuffer(m_indexBuffer);
        cmd->drawIndexed(m_indexCount);
    }
}
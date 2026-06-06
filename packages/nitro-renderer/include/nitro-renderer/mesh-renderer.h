#pragma once
#include <nitro-geometry/mesh.h>
#include <nitro-geometry/mesh-transformation.h>
#include <nitro-rhi/rhi-buffer.h>
#include <nitro-rhi/rhi-device.h>
#include <nitro-rhi/rhi-command-buffer.h>

namespace nitro::renderer
{
    class MeshRenderer
    {
    public:
        MeshRenderer(const geometry::Mesh &mesh, rhi::RHIDevice *device);
        ~MeshRenderer();
        void draw(rhi::RHICommandBuffer *cmd);

    private:
        rhi::RHIBuffer *m_vertexBuffer;
        rhi::RHIBuffer *m_indexBuffer;
        uint32_t m_indexCount;
        rhi::RHIDevice *m_device;
    };
}
#pragma once
#include "mesh-renderer.h"
#include <nitro-geometry/mesh-transformation.h>
#include <nitro-rhi/rhi-command-buffer.h>

namespace nitro::renderer
{
    class RenderObject
    {
    public:
        RenderObject(std::shared_ptr<MeshRenderer> meshRender, geometry::MeshTransformation transformation = geometry::MeshTransformation(glm::mat4(1.0f)));
        void draw(rhi::RHICommandBuffer *cmd, void *pushConstantOverride = nullptr, size_t size = 0);

        geometry::MeshTransformation transformation;

    private:
        std::shared_ptr<MeshRenderer> m_renderer;
    };
} // namespace nitro::renderer

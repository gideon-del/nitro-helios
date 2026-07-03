#pragma once
#include "mesh-renderer.h"
#include <nitro-geometry/mesh-transformation.h>
#include <nitro-rhi/rhi-command-buffer.h>

namespace nitro::renderer
{

    struct MaterialParameter
    {
        float metallic;
        float roughness;
    };

    struct RenderObjectPushConstant
    {
        glm::mat4 model = glm::mat4{1.0f};
        glm::mat4 normalMatrix = glm::mat4{1.0f};
        float metallic = 0.0;
        float roughness = 0.5;
    };
    class RenderObject
    {
    public:
        RenderObject(std::shared_ptr<MeshRenderer> meshRender, geometry::MeshTransformation transformation = geometry::MeshTransformation(glm::mat4(1.0f)), MaterialParameter material = MaterialParameter{0.0, 0.5});
        void draw(rhi::RHICommandBuffer *cmd, void *pushConstantOverride = nullptr, size_t size = 0);

        geometry::MeshTransformation transformation;
        MaterialParameter material;

    private:
        std::shared_ptr<MeshRenderer> m_renderer;
    };
} // namespace nitro::renderer

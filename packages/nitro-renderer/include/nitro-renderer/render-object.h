#pragma once
#include "mesh-renderer.h"
#include <nitro-geometry/mesh-transformation.h>
#include <nitro-rhi/rhi-command-buffer.h>
#include <nitro-renderer/material-system.h>
namespace nitro::renderer
{

    struct RenderObjectPushConstant
    {
        glm::mat4 model = glm::mat4{1.0f};
        glm::mat4 normalMatrix = glm::mat4{1.0f};
        glm::vec4 baseColor = glm::vec4(1.0f);
        float metallic = 0.0;
        float roughness = 0.5;
        uint useTextures = 0;
    };
    class RenderObject
    {
    public:
        RenderObject(std::shared_ptr<MeshRenderer> meshRender, geometry::MeshTransformation transformation = geometry::MeshTransformation(glm::mat4(1.0f)), std::shared_ptr<Material> material = nullptr);
        void draw(rhi::RHICommandBuffer *cmd, void *pushConstantOverride = nullptr, size_t size = 0, rhi::RHIDescriptorSet *defaultMaterialDescriptorSet = nullptr);
        void drawVertexOnly(rhi::RHICommandBuffer *cmd, void *pushConstantOverride = nullptr, size_t size = 0);

        geometry::MeshTransformation transformation;
        std::shared_ptr<Material> material;

    private:
        std::shared_ptr<MeshRenderer> m_renderer;
    };
} // namespace nitro::renderer

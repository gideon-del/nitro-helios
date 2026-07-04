#pragma once
#include "mesh-renderer.h"
#include <nitro-geometry/mesh-transformation.h>
#include <nitro-rhi/rhi-command-buffer.h>

namespace nitro::renderer
{

    struct Material
    {
        rhi::RHITexture *baseColor = nullptr;
        rhi::RHITexture *normal = nullptr;
        rhi::RHITexture *metallicRoughness = nullptr;
        rhi::RHITexture *ao = nullptr;
        rhi::RHITexture *emissive = nullptr;

        float metallicFactor = 0.0;
        float roughnessFactor = 0.5;
        glm::vec4 baseColorFactor{1.0f};

        rhi::RHIDescriptorSet *descriptorSet;
        bool hasTextures() const
        {
            return baseColor != nullptr;
        }
    };

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

        geometry::MeshTransformation transformation;
        std::shared_ptr<Material> material;

    private:
        std::shared_ptr<MeshRenderer> m_renderer;
    };
} // namespace nitro::renderer

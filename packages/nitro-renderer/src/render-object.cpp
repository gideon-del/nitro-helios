#include <nitro-renderer/render-object.h>

namespace nitro::renderer
{

    RenderObject::RenderObject(std::shared_ptr<MeshRenderer> meshRender, geometry::MeshTransformation transformation, std::shared_ptr<Material> material) : m_renderer(meshRender), transformation(transformation), material(material) {}

    void RenderObject::draw(rhi::RHICommandBuffer *cmd, void *pushConstantOverride, size_t size, rhi::RHIDescriptorSet *defaultMaterialDescriptorSet)
    {
        if (material != nullptr && material->descriptorSet != nullptr)
        {
            cmd->bindDescriptorSet(material->descriptorSet, 1);
        }
        else if (defaultMaterialDescriptorSet != nullptr)
        {
            cmd->bindDescriptorSet(defaultMaterialDescriptorSet, 1);
        }

        if (pushConstantOverride != nullptr)
        {
            cmd->setPushConstant(pushConstantOverride, size, 1);
        }
        else
        {
            auto model = transformation.getTransform();
            RenderObjectPushConstant pc;
            pc.model = model.model;
            pc.normalMatrix = model.normalMatrix;
            if (material != nullptr)
            {
                pc.metallic = material->metallicFactor;
                pc.roughness = material->roughnessFactor;
                pc.useTextures = 1;
                pc.baseColor = material->baseColorFactor;
            }
            cmd->setPushConstant(&pc, sizeof(RenderObjectPushConstant), 1);
        }
        m_renderer->draw(cmd);
    }

    void RenderObject::drawVertexOnly(rhi::RHICommandBuffer *cmd, void *pushConstantOverride, size_t size)
    {
        if (pushConstantOverride != nullptr)
        {
            cmd->setPushConstant(pushConstantOverride, size, 1);
        }
        else
        {
            auto model = transformation.getTransform();
            RenderObjectPushConstant pc;
            pc.model = model.model;
            pc.normalMatrix = model.normalMatrix;
            if (material != nullptr)
            {
                pc.metallic = material->metallicFactor;
                pc.roughness = material->roughnessFactor;
                pc.useTextures = material->hasTextures() ? 1 : 0;
                pc.baseColor = material->baseColorFactor;
            }
            cmd->setPushConstant(&pc, sizeof(RenderObjectPushConstant), 1);
        }
        m_renderer->draw(cmd);
    }
} // namespace nitro::renderer

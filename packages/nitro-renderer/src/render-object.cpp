#include <nitro-renderer/render-object.h>

namespace nitro::renderer
{

    RenderObject::RenderObject(std::shared_ptr<MeshRenderer> meshRender, geometry::MeshTransformation transformation, MaterialParameter material) : m_renderer(meshRender), transformation(transformation), material(material) {}

    void RenderObject::draw(rhi::RHICommandBuffer *cmd, void *pushConstantOverride, size_t size)
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
            pc.metallic = material.metallic;
            pc.roughness = material.roughness;
            cmd->setPushConstant(&pc, sizeof(RenderObjectPushConstant), 1);
        }
        m_renderer->draw(cmd);
    }

} // namespace nitro::renderer

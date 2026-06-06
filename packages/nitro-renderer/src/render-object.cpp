#include <nitro-renderer/render-object.h>

namespace nitro::renderer
{
    RenderObject::RenderObject(std::shared_ptr<MeshRenderer> meshRender, geometry::MeshTransformation transformation) : m_renderer(meshRender), transformation(transformation) {}

    void RenderObject::draw(rhi::RHICommandBuffer *cmd, void *pushConstantOverride, size_t size)
    {
        if (pushConstantOverride != nullptr)
        {
            cmd->setPushConstant(pushConstantOverride, size, 1);
        }
        else
        {
            auto model = transformation.getTransform();
            cmd->setPushConstant(&model, sizeof(geometry::PushConstant), 1);
        }
        m_renderer->draw(cmd);
    }

} // namespace nitro::renderer

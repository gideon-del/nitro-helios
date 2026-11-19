#include "nitro-renderer/scene-renderer.h"
#include "nitro-renderer/stage-outputs.h"
namespace nitro::renderer
{
    SceneRenderer::~SceneRenderer()
    {
        for (auto &[handle, pipeline] : m_viewPipelineLookup)
        {
            m_viewSystem.destroyView(handle);
        }
        m_viewPipelineLookup.clear();
    }

    frame::RenderViewHandle SceneRenderer::createView(const Extent2D &extent, std::unique_ptr<IRenderPipeline> pipeline)
    {
        assert(pipeline);

        auto viewHandle = m_viewSystem.createView(extent);

        m_viewPipelineLookup[viewHandle] = std::move(pipeline);

        return viewHandle;
    }

    void SceneRenderer::setViewPipeline(frame::RenderViewHandle view, std::unique_ptr<IRenderPipeline> pipeline)
    {
        assert(pipeline);
        auto it = m_viewPipelineLookup.find(view);
        if (it == m_viewPipelineLookup.end())
            return;

        m_viewSystem.resetView(view);

        it->second = std::move(pipeline);
    }

    void SceneRenderer::resize(frame::RenderViewHandle view, const Extent2D &extent)
    {
        m_viewSystem.resize(view, extent);
    }

    void SceneRenderer::destroyView(frame::RenderViewHandle view)
    {
        m_viewPipelineLookup.erase(view);
        m_viewSystem.destroyView(view);
    }

    rhi::RHITextureHandle SceneRenderer::render(frame::RenderViewHandle view, const RenderRequest &request, const frame::FrameInfo &frameInfo)
    {
        auto it = m_viewPipelineLookup.find(view);

        assert(it != m_viewPipelineLookup.end() && "View not found");
        auto pipeline = it->second.get();

        return m_viewSystem.render(view, frameInfo, [&](frame::RenderGraphBuilder &builder, const frame::RenderView &renderView)
                                   {
         RenderContext ctx{
            .builder = builder,
            .view = renderView,
            .request = request
         };

         pipeline->declare(ctx);
        return builder.blackboard().get<SceneColor>().texture; });
    }

} // namespace nitro::renderer

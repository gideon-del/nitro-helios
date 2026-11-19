#include "nitro-frame/view/view-system.h"

namespace nitro::frame
{
    RenderViewHandle ViewSystem::createView(const Extent2D &extent)
    {

        auto state = std::make_unique<ViewState>();
        state->view.size = extent;

        auto handle = m_renderViews.emplace(std::move(state));

        m_renderViews.get(handle)->get()->view.handle = handle;

        return handle;
    };

    const RenderView *ViewSystem::getView(const RenderViewHandle handle) const
    {
        const auto *view = m_renderViews.get(handle);

        if (!view)
        {
            return nullptr;
        }

        return &view->get()->view;
    }

    void ViewSystem::resize(const RenderViewHandle handle, const Extent2D &extent)
    {
        auto *view = m_renderViews.get(handle);

        if (!view)
        {
            return;
        }

        if (view->get()->view.size == extent)
            return;

        view->get()->view.size = extent;
        view->get()->view.historyValid = false;
        view->get()->historyLost = true;
    }

    void ViewSystem::destroyView(const RenderViewHandle handle)
    {
        if (!m_renderViews.get(handle))
            return;
        m_resourceManager.destroyViewResources(handle);
        m_renderViews.free(handle);
    };
    void ViewSystem::resetView(RenderViewHandle handle)
    {
        auto viewState = m_renderViews.get(handle);
        if (!viewState)
            return;

        viewState->get()->structureHash = 0;
        viewState->get()->compiled = false;
        viewState->get()->historyLost = true;
        viewState->get()->lastFrameNumber = UINT64_MAX;
        viewState->get()->view.historyValid = false;

        m_resourceManager.destroyViewResources(handle);
    };

    rhi::RHITextureHandle ViewSystem::render(const RenderViewHandle viewHandle, const FrameInfo &info, const ViewBuildFn &build)
    {
        auto viewState = m_renderViews.get(viewHandle);

        assert(viewState && "View Handle is stale or invalid");
        assert(build && "Build must not be null");

        auto *view = viewState->get();

        assert(view->lastFrameNumber != info.frameNumber && "View rendered twice in a frame");

        view->view.historyValid = !view->historyLost &&
                                  view->lastFrameNumber != UINT64_MAX &&
                                  info.frameNumber - view->lastFrameNumber == 1;
        RenderGraphBuilder graphBuilder(m_resourceManager);

        auto logicalOutput = build(graphBuilder, view->view);

        auto logical = graphBuilder.get(logicalOutput);
        assert(logical && "Invalid output handle");
        auto &outputInfo = m_resourceManager.info(logical->physicalHandle);

        assert(outputInfo.options.lifetime == ResourceLifetimeType::Persistent && "Output must have a persistent lifetime");

        auto hash = graphBuilder.structureHash();

        if (hash != view->structureHash || !view->compiled)
        {
            view->compiledGraph = graphBuilder.compile();
            view->compiled = true;
        }

        frame::ViewBuildInfo buildInfo{
            .handle = viewHandle,
            .extent = view->view.size,
            .structureHash = hash,
            .lifetimes = view->compiledGraph.lifetimes};

        m_resourceManager.build();
        if (m_resourceManager.buildViewResources(buildInfo))
            view->view.historyValid = false;

        view->structureHash = hash;

        ResourceFrameInfo frameInfo{
            .rhiFrame = info.frameIndex,
            .viewFrame = view->view.frameIdx,
            .viewHandle = viewHandle};
        GraphExecuteInfo executeInfo{
            .cmd = info.cmd,
            .view = view->view,
            .frameInfo = frameInfo,
            .timer = info.timer};
        graphBuilder.execute(view->compiledGraph, executeInfo);

        auto outputTexture = m_resourceManager.get(logical->physicalHandle, frameInfo);
        assert(outputTexture && "Texture not built");
        rhi::TextureBarrier barrier;
        barrier.texture = outputTexture;
        barrier.to = rhi::ResourceState::ShaderRead;

        info.cmd.barrier({{barrier}});

        view->lastFrameNumber = info.frameNumber;
        view->view.frameIdx++;
        view->historyLost = false;
        return outputTexture->getHandle();
    }

} // namespace nitro::frame

#pragma once
#include "nitro-frame/resources/resource-manager.h"
#include "nitro-frame/resources/resource-types.h"
#include "nitro-frame/graph/render-graph.h"
#include "nitro-frame/view/render-view.h"
#include "nitro-core/types/pool.h"
#include <functional>
#include <memory>
#include <cstdint>
namespace nitro::frame
{
    struct FrameInfo
    {
        rhi::RHICommandBuffer &cmd;
        uint32_t frameIndex;
        uint64_t frameNumber;
        rhi::RHITimer *timer = nullptr;
    };

    using ViewBuildFn = std::function<LogicalTextureHandle(RenderGraphBuilder &, const RenderView &)>;
    class ViewSystem
    {

        struct ViewState
        {
            RenderView view;
            size_t structureHash = 0;
            bool compiled = false;

            CompiledRenderGraph compiledGraph;

            uint64_t lastFrameNumber = UINT64_MAX;
            bool historyLost = true;
        };

        ResourcePool<std::unique_ptr<ViewState>, RenderViewTag> m_renderViews;
        ResourceManager &m_resourceManager;

    public:
        explicit ViewSystem(ResourceManager &resourceManager) : m_resourceManager(resourceManager) {}

        RenderViewHandle createView(const Extent2D &extent);

        const RenderView *getView(const RenderViewHandle handle) const;

        void resize(const RenderViewHandle handle, const Extent2D &extent);

        void destroyView(const RenderViewHandle handle);

        void resetView(RenderViewHandle handle);

        rhi::RHITextureHandle render(const RenderViewHandle viewHandle, const FrameInfo &info, const ViewBuildFn &build);
    };
} // namespace nitro::frame

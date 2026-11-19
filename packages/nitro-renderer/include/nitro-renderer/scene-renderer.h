#pragma once
#include "nitro-frame/resources/resource-manager.h"
#include "nitro-frame/view/view-system.h"
#include "nitro-renderer/render-pipeline/render-pipeline.h"
#include "nitro-renderer/render-context.h"
#include "nitro-rhi/rhi-device.h"
#include <unordered_map>
#include <memory>

namespace nitro::renderer
{
    class SceneRenderer
    {
    public:
        explicit SceneRenderer(rhi::RHIDevice &device) : m_resourceManager(device), m_viewSystem(m_resourceManager) {}
        ~SceneRenderer();

        frame::RenderViewHandle createView(const Extent2D &extent, std::unique_ptr<IRenderPipeline> pipeline);
        void setViewPipeline(frame::RenderViewHandle view, std::unique_ptr<IRenderPipeline> pipeline);
        void resize(frame::RenderViewHandle view, const Extent2D &extent);

        void destroyView(frame::RenderViewHandle view);

        rhi::RHITextureHandle render(frame::RenderViewHandle view, const RenderRequest &request, const frame::FrameInfo &frameInfo);

        SceneRenderer(const SceneRenderer &) = delete;
        SceneRenderer &operator=(const SceneRenderer &) = delete;

    private:
        frame::ResourceManager m_resourceManager;
        frame::ViewSystem m_viewSystem;

        std::unordered_map<frame::RenderViewHandle, std::unique_ptr<IRenderPipeline>, frame::RenderViewHandleHash> m_viewPipelineLookup;
    };
} // namespace nitro::renderer

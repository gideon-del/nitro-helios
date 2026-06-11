#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/passes/render-passes.h>
#include <nitro-renderer/interface/renderer.h>
#include <nitro-renderer/panels.h>

namespace nitro::renderer
{

    class DeferredRenderer : public IRenderer
    {
    public:
        DeferredRenderer(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<rhi::RHISwapchain> swapchain, std::string shaderDir, bool isMetal);
        void execute(rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings) override;

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::shared_ptr<rhi::RHISwapchain> m_swapchain;
        std::shared_ptr<GeometryPass> m_geometryPass;
        std::shared_ptr<CascadeShadowMapPass> m_csmPass;
        std::shared_ptr<DeferredLightingPass> m_deferredLightingPass;
        bool m_isMetal;
        ShadowPanel m_shadowPanel;
        LightPanel m_lightPanel;
        RendererPanel m_rendererPanel;
    };
} // namespace nitro::renderer

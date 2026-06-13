#pragma once
#include <nitro-renderer/interface/renderer.h>
#include <nitro-renderer/passes/render-passes.h>
#include <nitro-renderer/frame-resource.h>
#include <nitro-rhi/rhi-device.h>
#include <nitro-rhi/rhi-pipeline.h>
#include <nitro-rhi/rhi-descriptor-layout.h>
#include <nitro-renderer/panels.h>
namespace nitro::renderer
{
    class ForwardRenderer : public IRenderer
    {
    public:
        ForwardRenderer(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<rhi::RHISwapchain> swapchain, std::string shaderDir, bool isMetal);
        void execute(rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings) override;
        void resize(uint32_t width, uint32_t height) override;

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::shared_ptr<rhi::RHISwapchain> m_swapchain;
        std::shared_ptr<ForwardLightingPass> m_forwardLightPass;
        std::shared_ptr<CascadeShadowMapPass> m_csmPass;
        bool m_isMetal;
        ShadowPanel m_shadowPanel;
        LightPanel m_lightPanel;
        RendererPanel m_rendererPanel;
        StatPanel m_statsPanel;
    };
} // namespace nitro::renderer

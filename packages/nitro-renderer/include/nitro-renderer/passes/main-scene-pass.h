#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/panels.h>
#include <nitro-renderer/settings.h>
#include <nitro-renderer/render-graph.h>

namespace nitro::renderer
{
    class MainScenePass
    {
    public:
        MainScenePass(std::shared_ptr<rhi::RHIDevice> device,
                      std::shared_ptr<rhi::RHISwapchain> swapchain,
                      std::string shaderDir,
                      bool isMetal);
        ~MainScenePass();

        void execute(rhi::RHICommandBuffer *cmd, rhi::RHIRenderPassDesc &desc, RendererSettings &settings, rhi::RHITexture *inputTexture, RenderGraph &renderGraph);

    private:
        ShadowPanel m_shadowPanel;
        LightPanel m_lightPanel;
        RendererPanel m_rendererPanel;
        StatPanel m_statsPanel;
        ToneMapPanel m_tonemapPanel;
        BloomPanel m_bloomPanel;
        ColorGradingPanel m_colorGradePanel;
        SSAOPanel m_ssaoPanel;
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::shared_ptr<rhi::RHISwapchain> m_swapchain;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        rhi::RHIDescriptorSet *m_descriptorSet;
        rhi::RHITexture *m_lastInputTexture = nullptr;
    };
} // namespace nitro::renderer

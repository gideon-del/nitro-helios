#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/panels.h>
#include <nitro-renderer/settings.h>
#include <nitro-renderer/render-graph.h>
#include <nitro-renderer/per-frame.h>
#include <nitro-renderer/single-texture-pass-resource.h>
#include <nitro-renderer/particle-emitter-system.h>

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

        void execute(rhi::RHICommandBuffer *cmd, rhi::RHIRenderPassDesc &desc, RendererSettings &settings, rhi::RHITexture *inputTexture, RenderGraph &renderGraph, ParticleEmitterSystem &system, rhi::RHIBuffer *emitterBuffer);

    private:
        ShadowPanel m_shadowPanel;
        LightPanel m_lightPanel;
        RendererPanel m_rendererPanel;
        StatPanel m_statsPanel;
        ToneMapPanel m_tonemapPanel;
        BloomPanel m_bloomPanel;
        ColorGradingPanel m_colorGradePanel;
        SSAOPanel m_ssaoPanel;
        EmitterPanel m_emitterPanel;
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::shared_ptr<rhi::RHISwapchain> m_swapchain;
        rhi::RHIPipeline *m_pipeline;
        rhi::RHIDescriptorLayout *m_descriptorLayout;
        PerFrame<SingleInputPassResource> m_resources;
    };
} // namespace nitro::renderer

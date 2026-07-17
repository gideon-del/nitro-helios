#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/passes/render-passes.h>
#include <nitro-renderer/interface/renderer.h>
#include <nitro-renderer/post-process/post-process.h>
#include <nitro-renderer/panels.h>
namespace nitro::renderer
{
    class TiledDeferredRenderer : IRenderer
    {
    public:
        TiledDeferredRenderer(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<rhi::RHISwapchain> swapchain, std::string shaderDir, bool isMetal, std::shared_ptr<MaterialSystem> materialSystem);
        void execute(rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings) override;
        void resize(uint32_t width, uint32_t height) override;

    private:
        std::shared_ptr<rhi::RHIDevice> m_device;
        std::shared_ptr<rhi::RHISwapchain> m_swapchain;
        std::shared_ptr<GeometryPass> m_geometryPass;
        std::shared_ptr<CascadeShadowMapPass> m_csmPass;
        std::shared_ptr<DeferredLightingPass> m_deferredLightingPass;
        std::shared_ptr<DepthPrepass> m_depthPrepass;
        std::shared_ptr<TiledLightingComputePass> m_tileComputePass;
        std::shared_ptr<TileLightShadingPass> m_tileLightPass;
        std::shared_ptr<DebugDrawPass> m_debugDrawPass;
        std::shared_ptr<ToneMapPass> m_toneMapPass;
        std::shared_ptr<MainScenePass> m_mainScenePass;
        std::shared_ptr<SkyboxPass> m_skyboxPass;
        std::unique_ptr<BloomEffect> m_bloomEffect;
        std::unique_ptr<AutoExposurePass> m_autoExposurePass;
        rhi::RHITexture *m_cubemapTexture;
        rhi::RHITexture *m_irradianceTexture;
        rhi::RHITexture *m_prefilterMap;
        rhi::RHITexture *m_brdfLUT;
        bool m_isMetal;
        ShadowPanel m_shadowPanel;
        LightPanel m_lightPanel;
        RendererPanel m_rendererPanel;
        StatPanel m_statsPanel;
        std::shared_ptr<MaterialSystem> m_materialSystem;
    };
} // namespace nitro::renderer

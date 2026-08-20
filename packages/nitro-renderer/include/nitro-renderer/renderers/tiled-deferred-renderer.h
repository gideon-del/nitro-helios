#pragma once
#include <nitro-rhi/rhi.h>
#include <nitro-renderer/passes/render-passes.h>
#include <nitro-renderer/interface/renderer.h>
#include <nitro-renderer/post-process/post-process.h>
#include <nitro-renderer/panels.h>
#include <nitro-renderer/render-graph.h>
#include <nitro-renderer/particle-emitter-system.h>
namespace nitro::renderer
{
    class TiledDeferredRenderer : IRenderer
    {
    public:
        TiledDeferredRenderer(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<rhi::RHISwapchain> swapchain, std::string shaderDir, bool isMetal);
        void execute(rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings, rhi::RHITimer *timer) override;
        void resize(uint32_t width, uint32_t height) override;
        ~TiledDeferredRenderer();

    private:
        RenderGraph m_renderGraph;
        RGTextureID m_currentSceneTextureID;
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
        std::unique_ptr<ColorGradingPass> m_colorGradingPass;
        std::unique_ptr<SSAOPass> m_ssaoPass;
        std::unique_ptr<FXAAPass> m_fxaaPass;
        std::unique_ptr<ParticleUpdatePass> m_particleUpdatePass;
        std::unique_ptr<ParticleBillboardPass> m_particleBillboardPass;
        std::unique_ptr<ParticleEmitterPass> m_particleEmitterPass;
        std::unique_ptr<ParticleCompactPass> m_particleCompactPass;
        std::unique_ptr<ParticleIndirectPass> m_particleIndirectPass;
        std::unique_ptr<MeshCompactPass> m_meshCompactPass;
        std::unique_ptr<HiZMipPass> m_hizMipPass;
        std::unique_ptr<OcclusionCullingPass> m_occlusionCullPass;
        std::unique_ptr<CopyHizDepthPass> m_copyHizDepthPass;

        rhi::RHITexture *m_cubemapTexture;
        rhi::RHITexture *m_irradianceTexture;
        rhi::RHITexture *m_prefilterMap;
        rhi::RHITexture *m_brdfLUT;
        bool m_isMetal;
        ShadowPanel m_shadowPanel;
        LightPanel m_lightPanel;
        RendererPanel m_rendererPanel;
        StatPanel m_statsPanel;
        RGCompiledFrameGraph m_compiledFrameGraph;
        ParticleEmitterSystem m_emitterSystem;

        void buildRenderGraph();
    };
} // namespace nitro::renderer

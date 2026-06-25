#include <nitro-renderer/renderers/tiled-deferred-renderer.h>

namespace nitro::renderer
{
    TiledDeferredRenderer::TiledDeferredRenderer(
        std::shared_ptr<rhi::RHIDevice> device,
        std::shared_ptr<rhi::RHISwapchain> swapchain,
        std::string shaderDir,
        bool isMetal)
        : m_device(device),
          m_swapchain(swapchain),
          m_isMetal(isMetal)
    {
        m_depthPrepass = std::make_shared<DepthPrepass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal);
        m_geometryPass = std::make_shared<GeometryPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), m_depthPrepass->getDepthTexture(), shaderDir, isMetal);
        m_csmPass = std::make_shared<CascadeShadowMapPass>(m_device, shaderDir, isMetal);
        m_tileComputePass = std::make_shared<TiledLightingComputePass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), 1000, m_geometryPass->gBuffer, shaderDir, m_isMetal);
        m_tileLightPass = std::make_shared<TileLightShadingPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), m_geometryPass->gBuffer, m_tileComputePass->getFrameResources(), shaderDir, isMetal);
        m_deferredLightingPass = std::make_shared<DeferredLightingPass>(
            m_device,
            m_swapchain,
            m_csmPass->getCascadeTextures(),
            m_geometryPass->gBuffer,
            m_tileLightPass->getLightTexture(),
            shaderDir,
            isMetal);
    }
    void TiledDeferredRenderer::resize(uint32_t width, uint32_t height)
    {
        m_depthPrepass->resize(width, height);
        m_geometryPass->resize(width, height, m_depthPrepass->getDepthTexture());
        m_tileComputePass->resize(width, height, m_geometryPass->gBuffer);
        m_tileLightPass->resize(width, height, m_geometryPass->gBuffer, m_tileComputePass->getFrameResources());
        m_deferredLightingPass->recreate(m_geometryPass->gBuffer, m_tileLightPass->getLightTexture());
    };

    void TiledDeferredRenderer::execute(rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings)
    {

        CascadeShadowContext shadowCtx;
        shadowCtx.aspect = (float)m_swapchain->getWidth() / (float)m_swapchain->getHeight();
        shadowCtx.cameraFar = ctx.CAMERA_FAR;
        shadowCtx.cameraNear = ctx.CAMERA_NEAR;
        shadowCtx.fov = glm::radians(60.0f);
        shadowCtx.lambda = settings.shadow.lambda;
        shadowCtx.cameraView = ctx.camera->getView();
        shadowCtx.lightView = settings.light.lightCamera.getView();

        m_csmPass->execute(cmd, *ctx.scene, shadowCtx);

        GeometryCameraBuffer geometryCamera{};
        geometryCamera.view = ctx.camera->getView();
        geometryCamera.proj = glm::perspectiveRH_ZO(shadowCtx.fov, shadowCtx.aspect, shadowCtx.cameraNear, shadowCtx.cameraFar);
        if (!m_isMetal)
        {
            geometryCamera.proj[1][1] *= -1.0f;
        }
        DepthPrePassCamera depthCamera;
        depthCamera.view = geometryCamera.view;
        depthCamera.proj = geometryCamera.proj;
        m_depthPrepass->execute(cmd, *ctx.scene, depthCamera);
        m_geometryPass->execute(cmd, geometryCamera, *ctx.scene);

        TiledCameraUBO computeUBO;
        computeUBO.farPlane = ctx.CAMERA_FAR;
        computeUBO.nearPlane = ctx.CAMERA_NEAR;
        computeUBO.screenSize = glm::vec2(float(m_swapchain->getWidth()), float(m_swapchain->getHeight()));
        computeUBO.invProj = glm::inverse(geometryCamera.proj);
        computeUBO.view = geometryCamera.view;

        m_tileComputePass->execute(cmd, settings.light, computeUBO);

        TiledLightPassUBO lightPassUBO;

        lightPassUBO.invViewProj = glm::inverse(geometryCamera.proj * geometryCamera.view);
        lightPassUBO.numTilesX = static_cast<uint32_t>(
            std::ceil(float(m_swapchain->getWidth()) / 16.0f));
        lightPassUBO.screenSize = glm::vec2(float(m_swapchain->getWidth()), float(m_swapchain->getHeight()));
        lightPassUBO.showHeatMap = settings.selectedDebugMode == DebugMode::HeatMap ? 1 : 0;
        m_tileLightPass->execute(cmd, lightPassUBO);

        rhi::RHIRenderPassDesc rpDesc{};
        rpDesc.clearColor[0] = 0.3f;
        rpDesc.clearColor[1] = 0.3f;
        rpDesc.clearColor[2] = 0.3f;
        rpDesc.clearColor[3] = 1.0f;
        rpDesc.clearDepth = 1.0f;
        rpDesc.hasDepth = true;
        cmd->beginRenderPass(rpDesc);
        DeferredLightingFrameData frameData;
        frameData.ambient = settings.light.ambient;
        frameData.Ka = settings.light.Ka;
        frameData.Ks = settings.light.Ks;
        frameData.Kd = settings.light.Kd;
        frameData.shininess = settings.light.shininess;
        frameData.cascadeSplit = m_csmPass->cascadeSplit;
        for (int i = 0; i < CascadeShadowMapPass::CASCADE_COUNT; i++)
        {
            frameData.lightViewProj[i] = m_csmPass->lightViewProj[i];
        }
        frameData.invViewProj = glm::inverse(geometryCamera.proj * geometryCamera.view);
        frameData.view = geometryCamera.view;
        frameData.cameraPosition = glm::vec4(ctx.camera->getEye(), 1.0f);

        frameData.lightPosition = glm::vec4(settings.light.lightCamera.getEye(), 1.0f);
        frameData.lightColor = glm::vec4(settings.light.lightColor, 1.0f);
        frameData.shadowBias = settings.shadow.bias;
        frameData.shadowNormalBias = settings.shadow.normalBias;
        frameData.showCascadeColors = settings.shadow.showCascadeColors ? 1.0f : 0.0f;
        frameData.debugMode = static_cast<float>(settings.selectedDebugMode);
        for (int i = 0; i < settings.light.pointLights.size(); i++)
        {
            frameData.pointLights[i] = settings.light.pointLights[i];
        }

        m_deferredLightingPass->execute(cmd, frameData);

        m_device->beginImGuiFrame();
        m_lightPanel.draw(settings.light);
        m_shadowPanel.draw(settings.shadow);
        m_rendererPanel.draw(settings);
        m_statsPanel.draw(settings.stats);
        m_device->endImGuiFrame();
        m_device->drawImGui(cmd);
        cmd->endRenderPass();
    }
} // namespace nitro::renderer

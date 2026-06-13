#include <nitro-renderer/renderers/forward-renderer.h>

namespace nitro::renderer
{

    ForwardRenderer::ForwardRenderer(std::shared_ptr<rhi::RHIDevice> device, std::shared_ptr<rhi::RHISwapchain> swapchain, std::string shaderDir, bool isMetal) : m_device(device), m_swapchain(swapchain), m_isMetal(isMetal)
    {

        m_csmPass = std::make_shared<CascadeShadowMapPass>(m_device, shaderDir, isMetal);
        m_forwardLightPass = std::make_shared<ForwardLightingPass>(m_device, m_swapchain, m_csmPass->getCascadeTextures(), shaderDir, isMetal);
    }

    void ForwardRenderer::execute(rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings)
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
        rhi::RHIRenderPassDesc rpDesc{};
        rpDesc.clearColor[0] = 0.0f;
        rpDesc.clearColor[1] = 0.0f;
        rpDesc.clearColor[2] = 0.0f;
        rpDesc.clearColor[3] = 1.0f;
        rpDesc.clearDepth = 1.0f;
        rpDesc.hasDepth = true;
        cmd->beginRenderPass(rpDesc);
        FrameData frameData;
        frameData.ambient = settings.light.ambient;
        frameData.Ka = settings.light.Ka;
        frameData.Ks = settings.light.Ks;
        frameData.Kd = settings.light.Kd;
        frameData.cascadeSplit = m_csmPass->cascadeSplit;
        for (int i = 0; i < CascadeShadowMapPass::CASCADE_COUNT; i++)
        {
            frameData.lightViewProj[i] = m_csmPass->lightViewProj[i];
        }
        frameData.proj = glm::perspective(shadowCtx.fov, shadowCtx.aspect, shadowCtx.cameraNear, shadowCtx.cameraFar);
        if (!m_isMetal)
        {
            frameData.proj[1][1] *= -1.0f;
        }
        frameData.view = shadowCtx.cameraView;
        frameData.cameraPos = glm::vec4(ctx.camera->getEye(), 1.0f);
        frameData.lightPos = glm::vec4(settings.light.lightCamera.getEye(), 1.0f);
        frameData.lightColor = glm::vec4(settings.light.lightColor, 1.0f);
        frameData.shadowBias = settings.shadow.bias;
        frameData.shadowNormalBias = settings.shadow.normalBias;
        frameData.showCascadeColors = settings.shadow.showCascadeColors ? 1.0f : 0.0f;
        frameData.debugMode = static_cast<float>(settings.selectedDebugMode);
        for (int i = 0; i < settings.light.pointLights.size(); i++)
        {
            frameData.pointLights[i] = settings.light.pointLights[i];
        }

        m_forwardLightPass->execute(cmd, *ctx.scene, frameData);

        m_device->beginImGuiFrame();
        m_lightPanel.draw(settings.light);
        m_shadowPanel.draw(settings.shadow);
        m_rendererPanel.draw(settings);
        m_statsPanel.draw(settings.stats);
        m_device->endImGuiFrame();
        m_device->drawImGui(cmd);
        cmd->endRenderPass();
    }
    void ForwardRenderer::resize(uint32_t width, uint32_t height)
    {
    }

} // namespace nitro::renderer

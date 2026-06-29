#include <nitro-renderer/renderers/tiled-deferred-renderer.h>

namespace nitro::renderer
{

    TileFrustumCPU calculateTileFrustumCPU(
        glm::vec2 tile, glm::vec2 screenSize,
        const glm::mat4 &invProj,
        float tileNear, float tileFar)
    {
        auto reconstructCorner = [&](glm::vec2 px)
        {
            glm::vec2 ndc = (px * glm::vec2(16.0f) / screenSize) * 2.0f - 1.0f;
            glm::vec4 clip = invProj * glm::vec4(ndc, 1.0f, 1.0f);
            return glm::normalize(glm::vec3(clip) / clip.w);
        };

        glm::vec3 topLeft = reconstructCorner(tile);
        glm::vec3 topRight = reconstructCorner(tile + glm::vec2(1, 0));
        glm::vec3 bottomRight = reconstructCorner(tile + glm::vec2(1, 1));
        glm::vec3 bottomLeft = reconstructCorner(tile + glm::vec2(0, 1));

        TileFrustumCPU frustum;
        frustum.topNormal = glm::normalize(glm::cross(topLeft, topRight));
        frustum.rightNormal = glm::normalize(glm::cross(topRight, bottomRight));
        frustum.bottomNormal = glm::normalize(glm::cross(bottomRight, bottomLeft));
        frustum.leftNormal = glm::normalize(glm::cross(topLeft, bottomLeft));
        frustum.tileNear = tileNear;
        frustum.tileFar = tileFar;

        return frustum;
    }

    void visualizeTileRay(std::shared_ptr<DebugDrawPass> debugDrawPass,
                          glm::vec2 tile,
                          glm::vec2 screenSize,
                          const glm::mat4 &invProj,
                          const glm::mat4 &view,
                          float tileNear,
                          float tileFar)
    {

        auto reconstructCorner = [&](glm::vec2 px)
        {
            glm::vec2 ndc = (px * glm::vec2(16.0f) / screenSize) * 2.0f - 1.0f;
            glm::vec4 clip = invProj * glm::vec4(ndc, 1.0f, 1.0f);
            return glm::normalize(glm::vec3(clip) / clip.w);
        };

        glm::mat4 invView = glm::inverse(view);

        glm::vec3 eye = glm::vec3(invView[3]);

        auto toWorldDir = [&](glm::vec3 d)
        {
            return glm::normalize(
                glm::vec3(invView * glm::vec4(d, 0)));
        };

        glm::vec3 topLeft = reconstructCorner(tile);
        glm::vec3 topRight = reconstructCorner(tile + glm::vec2(1, 0));
        glm::vec3 bottomRight = reconstructCorner(tile + glm::vec2(1, 1));
        glm::vec3 bottomLeft = reconstructCorner(tile + glm::vec2(0, 1));

        glm::vec3 forward = glm::normalize(-glm::vec3(invView[2]));
        // glm::vec3 right = glm::normalize(glm::vec3(invView[0]));
        // glm::vec3 up = glm::normalize(glm::vec3(invView[1]));

        // debugDrawPass->drawRay(
        //     eye,
        //     forward,
        //     20.0f,
        //     glm::vec3(1, 0, 0));

        glm::vec3 origin =
            eye +
            forward;

        // debugDrawPass->drawRay(origin, forward, 50.0f, {1, 0, 0});
        // debugDrawPass->drawRay(origin, -forward, 50.0f, {0, 1, 0});

        // debugDrawPass->drawRay(origin, forward, 20.0f, {0, 0, 1});
        // debugDrawPass->drawRay(origin, right, 1.0f, {1, 0, 0});
        // debugDrawPass->drawRay(origin, up, 1.0f, {0, 1, 0});
        // debugDrawPass->drawAxes(glm::translate(glm::mat4(1.0f), origin), 2.0f);

        // glm::vec3 center =
        //     reconstructCorner(tile + glm::vec2(0.5f, 0.5f));

        // debugDrawPass->drawRay(
        //     origin,
        //     toWorldDir(center),
        //     200.0f,
        //     {1, 1, 1});
        // debugDrawPass->drawRay(
        //     origin,
        //     toWorldDir(topLeft),
        //     20.0f,
        //     glm::vec3(1, 0, 0));
        // debugDrawPass->drawRay(
        //     origin,
        //     toWorldDir(topRight),
        //     20.0f,
        //     glm::vec3(0, 1, 0));

        // debugDrawPass->drawRay(
        //     origin,
        //     toWorldDir(bottomLeft),
        //     20.0f,
        //     glm::vec3(0, 0, 1));

        // debugDrawPass->drawRay(
        //     origin,
        //     toWorldDir(bottomRight),
        //     20.0f,
        //     glm::vec3(1, 1, 0));
    };

    void visualizeTileFrustumTest(
        std::shared_ptr<DebugDrawPass> debug,
        glm::vec2 tile,
        glm::vec2 screenSize,
        const glm::mat4 &invProj,
        const glm::mat4 &view,
        const glm::vec3 &testLightWorldPos)
    {
        // Placeholder bounds — only the four normals matter for this test
        TileFrustumCPU frustum = calculateTileFrustumCPU(
            tile, screenSize, invProj, 1.0f, 50.0f);

        // Frustum center, in world space — the camera's own eye position
        glm::mat4 invView = glm::inverse(view);
        glm::vec3 cameraForward = -glm::vec3(invView[2]);
        glm::vec3 frustumCenterWorld = glm::vec3(invView[3]) + cameraForward * 5.0f;

        // Test light, brought into view space — same space the normals live in
        glm::vec3 lightViewPos = glm::vec3(view * glm::vec4(testLightWorldPos, 1.0f));

        auto sideColor = [&](const glm::vec3 &normal)
        {
            float side = glm::dot(normal, lightViewPos);
            return side >= 0.0f
                       ? glm::vec3(0, 1, 0)  // green — light on the "inside" side
                       : glm::vec3(1, 0, 0); // red   — light on the "outside" side
        };

        bool isInTop = glm::dot(frustum.topNormal, lightViewPos) >= 0.0;
        bool isInBottom = glm::dot(frustum.bottomNormal, lightViewPos) >= 0.0;
        bool isInRight = glm::dot(frustum.rightNormal, lightViewPos) >= 0.0;
        bool isInLeft = glm::dot(frustum.rightNormal, lightViewPos) >= 0.0;

        // Transform each normal (a direction — w=0, never divided) back to world space
        auto normalToWorld = [&](const glm::vec3 &n)
        {
            return glm::vec3(invView * glm::vec4(n, 0.0f));
        };

        debug->drawRay(frustumCenterWorld, normalToWorld(frustum.topNormal), 5.0f, sideColor(frustum.topNormal));
        debug->drawRay(frustumCenterWorld, normalToWorld(frustum.bottomNormal), 5.0f, sideColor(frustum.bottomNormal));
        debug->drawRay(frustumCenterWorld, normalToWorld(frustum.leftNormal), 5.0f, sideColor(frustum.leftNormal));
        debug->drawRay(frustumCenterWorld, normalToWorld(frustum.rightNormal), 5.0f, sideColor(frustum.rightNormal));
    }

    void visualizeTileDepthRange(
        std::shared_ptr<DebugDrawPass> debug,
        const glm::vec3 &cameraEye,
        const glm::vec3 &cameraForward,
        float tileNear,
        float tileFar,
        const glm::vec3 &color)
    {
        glm::vec3 nearPoint = cameraEye + cameraForward * tileNear;
        glm::vec3 farPoint = cameraEye + cameraForward * tileFar;

        debug->drawAABB(nearPoint - glm::vec3(0.3f), nearPoint + glm::vec3(0.3f), glm::vec3(0, 1, 0)); // green = near
        debug->drawAABB(farPoint - glm::vec3(0.3f), farPoint + glm::vec3(0.3f), glm::vec3(1, 0, 0));   // red = far

        debug->drawLine(nearPoint, farPoint, glm::vec3(1, 1, 0));
    }
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
        m_debugDrawPass = std::make_shared<DebugDrawPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, m_isMetal);
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
        lightPassUBO.view = geometryCamera.view;
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
        // glm::mat4 testTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0, 5, 0));
        // m_debugDrawPass->drawAxes(testTransform, 5.0f);
        // auto invView = glm::inverse(ctx.camera->getView());
        // glm::vec3 cameraForward = -glm::vec3(invView[2]);
        // glm::vec3 testOrigin = glm::vec3(invView[3]) + cameraForward * 5.0f;
        // m_debugDrawPass->drawAxes(glm::translate(invView, glm::vec3(0, 0, -1.0f)), 2.0f);
        // m_debugDrawPass->drawPlane(glm::vec3(0, 5, 0), glm::vec3(0, 1, 0), 5.0f, glm::vec4(1, 0.5, 0, 1));

        // m_debugDrawPass->drawAABB(glm::vec3(-2, 0, -2), glm::vec3(2, 4, 2), glm::vec3(0, 1, 1));
        // m_debugDrawPass->drawRay(glm::vec3(0, 10, 0), glm::vec3(0, 1, 0), 12.0f, glm::vec3(1, 1, 0));
        // glm::vec4 cascadeColors[4] = {
        //     {1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}, {1, 1, 0, 1}};

        // for (int i = 0; i < CascadeShadowMapPass::CASCADE_COUNT; i++)
        // {
        //     glm::mat4 invLightVP = glm::inverse(m_csmPass->lightViewProj[i]);
        //     m_debugDrawPass->drawFrustum(invLightVP, 0.0f, 1.0f, cascadeColors[i]);
        // }

        glm::vec2 testTile(0, 0);
        glm::vec2 testTile2(100, 0);
        glm::vec2 testTile3(100, 75);
        glm::vec2 testTile4(0, 75);
        // glm::mat4 invView = glm::inverse(ctx.camera->getView());
        // glm::vec3 cameraForward = -glm::vec3(invView[2]);
        // glm::vec3 lightInside = ctx.camera->getEye() + cameraForward * 20.0f;
        // glm::vec3 lightOutside = ctx.camera->getEye() - cameraForward * 20.0f;

        // // visualizeTileFrustumTest(m_debugDrawPass, testTile, lightPassUBO.screenSize, computeUBO.invProj, computeUBO.view, lightOutside);
        // // visualizeTileFrustumTest(m_debugDrawPass, testTile, lightPassUBO.screenSize, computeUBO.invProj, computeUBO.view, lightOutside);
        // visualizeTileDepthRange(m_debugDrawPass, ctx.camera->getEye(), cameraForward, 3.0f, 15.0f, glm::vec3(0.0, 0.0, 1.0));

        // for (const auto &light : settings.light.pointLights)
        // {
        //     glm::vec3 p = glm::vec3(light.position);

        //     m_debugDrawPass->drawSphere(
        //         p,
        //         10.0,
        //         glm::vec3(0.0, 0, 1.0));
        // }
        // m_debugDrawPass->drawTileFrustum(testTile, lightPassUBO.screenSize, computeUBO.invProj, computeUBO.view, 400, 500);
        // auto viewProj = geometryCamera.proj * geometryCamera.view;
        // m_debugDrawPass->execute(cmd, viewProj);

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

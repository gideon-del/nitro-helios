// #include <nitro-renderer/renderers/deferred-renderer.h>
// #include <nitro-renderer/utils.h>

// namespace nitro::renderer
// {
//     DeferredRenderer::DeferredRenderer(
//         std::shared_ptr<rhi::RHIDevice> device,
//         std::shared_ptr<rhi::RHISwapchain> swapchain,
//         std::string shaderDir,
//         bool isMetal,
//         std::shared_ptr<MaterialSystem> materialSystem)
//         : m_device(device),
//           m_swapchain(swapchain),
//           m_isMetal(isMetal),
//           m_materialSystem(materialSystem)
//     {
//         rhi::RHITexture *hdrTexture = loadHDRImage(m_device, "./assets/modern_evening_street.hdr");
//         m_cubemapTexture = createCubeMap(m_device, hdrTexture, 512, shaderDir, isMetal);
//         m_irradianceTexture = generateIrradianceMap(m_device, m_cubemapTexture, 32, shaderDir, isMetal);
//         m_prefilterMap = generatePrefliteredMap(m_device, m_cubemapTexture, 512, shaderDir, isMetal);
//         m_brdfLUT = generateBrdfLUT(m_device, 512, shaderDir, isMetal);
//         m_skyboxPass = std::make_shared<SkyboxPass>(m_device, m_cubemapTexture, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal);
//         m_depthPrepass = std::make_shared<DepthPrepass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal, m_materialSystem);
//         m_geometryPass = std::make_shared<GeometryPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), m_depthPrepass->getDepthTexture(), shaderDir, isMetal, m_materialSystem);
//         m_ssaoPass = std::make_unique<SSAOPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), m_geometryPass->gBuffer.depth, m_geometryPass->gBuffer.normal, shaderDir, isMetal);
//         m_csmPass = std::make_shared<CascadeShadowMapPass>(m_device, shaderDir, isMetal);
//         m_lightStencilPass = std::make_shared<LightingStencilPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), m_geometryPass->gBuffer, shaderDir, m_isMetal);
//         m_deferredLightingPass = std::make_shared<DeferredLightingPass>(
//             m_device,
//             m_swapchain->getWidth(),
//             m_swapchain->getHeight(),
//             m_csmPass->getCascadeTextures(),
//             m_geometryPass->gBuffer,
//             m_cubemapTexture,
//             m_lightStencilPass->getLightingTexture(),
//             m_skyboxPass->getSkyboxTexture(),
//             m_brdfLUT,
//             m_prefilterMap,
//             m_ssaoPass->getSSAOTexture(),
//             shaderDir,
//             isMetal);
//         m_bloomEffect = std::make_unique<BloomEffect>(m_device, m_swapchain->getWidth(),
//                                                       m_swapchain->getHeight(), shaderDir, isMetal);
//         m_toneMapPass = std::make_shared<ToneMapPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal);
//         m_autoExposurePass = std::make_unique<AutoExposurePass>(m_device, m_swapchain->getWidth(),
//                                                                 m_swapchain->getHeight(), shaderDir, isMetal);
//         m_mainScenePass = std::make_shared<MainScenePass>(m_device, m_swapchain, m_toneMapPass->getToneMappedTexture(), shaderDir, isMetal);
//     }

//     void DeferredRenderer::execute(rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings)
//     {

//         CascadeShadowContext shadowCtx;
//         shadowCtx.aspect = (float)m_swapchain->getWidth() / (float)m_swapchain->getHeight();
//         shadowCtx.cameraFar = ctx.CAMERA_FAR;
//         shadowCtx.cameraNear = ctx.CAMERA_NEAR;
//         shadowCtx.fov = glm::radians(60.0f);
//         shadowCtx.lambda = settings.shadow.lambda;
//         shadowCtx.cameraView = ctx.camera->getView();
//         shadowCtx.lightView = settings.light.lightCamera.getView();

//         m_csmPass->execute(cmd, *ctx.scene, shadowCtx);

//         GeometryCameraBuffer geometryCamera{};
//         geometryCamera.view = ctx.camera->getView();
//         geometryCamera.proj = glm::perspectiveRH_ZO(shadowCtx.fov, shadowCtx.aspect, shadowCtx.cameraNear, shadowCtx.cameraFar);
//         if (!m_isMetal)
//         {
//             geometryCamera.proj[1][1] *= -1.0f;
//         }
//         DepthPrePassCamera depthCamera;
//         depthCamera.view = geometryCamera.view;
//         depthCamera.proj = geometryCamera.proj;

//         SkyboxPassUBO skyboxUbo;
//         glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(geometryCamera.view));
//         skyboxUbo.viewProj = glm::inverse(geometryCamera.proj * viewNoTranslation);
//         skyboxUbo.screenSize = {float(m_swapchain->getWidth()), float(m_swapchain->getHeight())};
//         m_skyboxPass->execute(cmd, skyboxUbo);
//         m_depthPrepass->execute(cmd, *ctx.scene, depthCamera);
//         m_geometryPass->execute(cmd, geometryCamera, *ctx.scene, settings.light);
//         SSAOPushConstant ssaoPc;
//         ssaoPc.invProj = glm::inverse(geometryCamera.proj);
//         ssaoPc.view = geometryCamera.view;
//         ssaoPc.proj = geometryCamera.proj;
//         ssaoPc.textureSize = glm::vec2(float(m_swapchain->getWidth()), float(m_swapchain->getHeight()));
//         ssaoPc.totalSamples = static_cast<uint>(ctx.ssaoSamples.samples.size());
//         ssaoPc.radius = settings.ssao.radius;

//         m_ssaoPass->execute(cmd, ssaoPc, ctx.ssaoSamples.samples, settings.ssao.depthSigma);
//         LightStencilCamera lightStencilCamera;
//         lightStencilCamera.view = geometryCamera.view;
//         lightStencilCamera.proj = geometryCamera.proj;
//         lightStencilCamera.invViewProj = glm::inverse(geometryCamera.proj * geometryCamera.view);
//         m_lightStencilPass->execute(cmd, settings.light, lightStencilCamera);

//         DeferredLightingFrameData frameData;
//         frameData.ambient = settings.light.ambient;
//         frameData.Ka = settings.light.Ka;
//         frameData.Ks = settings.light.Ks;
//         frameData.Kd = settings.light.Kd;
//         frameData.shininess = settings.light.shininess;
//         frameData.cascadeSplit = m_csmPass->cascadeSplit;
//         for (int i = 0; i < CascadeShadowMapPass::CASCADE_COUNT; i++)
//         {
//             frameData.lightViewProj[i] = m_csmPass->lightViewProj[i];
//         }
//         frameData.invViewProj = glm::inverse(geometryCamera.proj * geometryCamera.view);
//         frameData.view = geometryCamera.view;
//         frameData.cameraPosition = glm::vec4(ctx.camera->getEye(), 1.0f);

//         frameData.lightPosition = glm::vec4(settings.light.lightCamera.getEye(), 1.0f);
//         frameData.lightColor = glm::vec4(settings.light.lightColor, 1.0f);
//         frameData.shadowBias = settings.shadow.bias;
//         frameData.shadowNormalBias = settings.shadow.normalBias;
//         frameData.showCascadeColors = settings.shadow.showCascadeColors ? 1.0f : 0.0f;
//         frameData.debugMode = static_cast<float>(settings.selectedDebugMode);
//         frameData.lightMode = static_cast<float>(settings.selectedLightMode);
//         frameData.roughness = settings.light.roughness;
//         for (int i = 0; i < settings.light.pointLights.size(); i++)
//         {
//             frameData.pointLights[i] = settings.light.pointLights[i];
//         }

//         m_deferredLightingPass->execute(cmd, frameData);
//         rhi::RHITexture *hdrTexture = m_deferredLightingPass->getLightTexture();
//         rhi::RHITexture *sceneTexture = hdrTexture;
//         if (settings.tonemap.autoExposure)
//         {
//             AutoExposurePushConstant autoExposurePc;
//             autoExposurePc.inputTextureSize = glm::vec2(float(m_swapchain->getWidth()), float(m_swapchain->getHeight()));

//             settings.tonemap.exposure = m_autoExposurePass->execute(cmd, autoExposurePc, hdrTexture, settings.tonemap, ctx.deltaTime);
//         }
//         if (settings.bloom.enable)
//         {

//             sceneTexture = m_bloomEffect->execute(cmd, sceneTexture, settings.bloom);
//         }
//         ToneMapPassUBO toneMapUBO;
//         toneMapUBO.exposure = settings.tonemap.exposure;
//         toneMapUBO.mode = static_cast<uint>(settings.tonemap.mode);
//         m_toneMapPass->execute(cmd, toneMapUBO, sceneTexture);
//         rhi::RHIRenderPassDesc rpDesc{};
//         rpDesc.clearColor[0] = 0.3f;
//         rpDesc.clearColor[1] = 0.3f;
//         rpDesc.clearColor[2] = 0.3f;
//         rpDesc.clearColor[3] = 1.0f;
//         rpDesc.clearDepth = 1.0f;
//         rpDesc.hasDepth = true;
//         m_mainScenePass->execute(cmd, rpDesc, settings);
//     }
//     void DeferredRenderer::resize(uint32_t width, uint32_t height)
//     {
//         m_depthPrepass->resize(width, height);
//         m_geometryPass->resize(width, height, m_depthPrepass->getDepthTexture());
//         m_ssaoPass->resize(width, height, m_geometryPass->gBuffer.depth, m_geometryPass->gBuffer.normal);
//         m_lightStencilPass->resize(width, height, m_geometryPass->gBuffer);
//         m_skyboxPass->resize(width, height, m_cubemapTexture);
//         m_deferredLightingPass->recreate(width, height, m_geometryPass->gBuffer, m_cubemapTexture, m_lightStencilPass->getLightingTexture(), m_skyboxPass->getSkyboxTexture(), m_brdfLUT,
//                                          m_prefilterMap, m_ssaoPass->getSSAOTexture());
//         m_bloomEffect->resize(width, height);
//         m_toneMapPass->resize(width, height);
//         m_autoExposurePass->resize(width, height);
//         m_mainScenePass->resize(m_toneMapPass->getToneMappedTexture());
//     };
// } // namespace nitro::renderer

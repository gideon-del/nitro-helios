#include <nitro-renderer/renderers/tiled-deferred-renderer.h>
#include <nitro-renderer/utils.h>

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

        glm::vec3 origin =
            eye +
            forward;
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
        rhi::RHITexture *hdrTexture = loadHDRImage(m_device, "./assets/flamingo_pan_2k.hdr");
        m_cubemapTexture = createCubeMap(m_device, hdrTexture, 512, shaderDir, isMetal);
        m_irradianceTexture = generateIrradianceMap(m_device, m_cubemapTexture, 64, shaderDir, isMetal);
        m_prefilterMap = generatePrefliteredMap(m_device, m_cubemapTexture, 512, shaderDir, isMetal);
        m_brdfLUT = generateBrdfLUT(m_device, 512, shaderDir, isMetal);

        m_device->destroyTexture(hdrTexture);

        m_skyboxPass = std::make_shared<SkyboxPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal);
        m_depthPrepass = std::make_shared<DepthPrepass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal);
        m_geometryPass = std::make_shared<GeometryPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal);

        m_ssaoPass = std::make_unique<SSAOPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal);
        m_csmPass = std::make_shared<CascadeShadowMapPass>(m_device, shaderDir, isMetal);
        m_tileComputePass = std::make_shared<TiledLightingComputePass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), 1000, shaderDir, m_isMetal);
        m_tileLightPass = std::make_shared<TileLightShadingPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal);
        m_deferredLightingPass = std::make_shared<DeferredLightingPass>(
            m_device,
            m_swapchain->getWidth(),
            m_swapchain->getHeight(),
            shaderDir,
            isMetal);
        m_debugDrawPass = std::make_shared<DebugDrawPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, m_isMetal);
        m_bloomEffect = std::make_unique<BloomEffect>(m_device, m_swapchain->getWidth(),
                                                      m_swapchain->getHeight(), shaderDir, isMetal);
        m_autoExposurePass = std::make_unique<AutoExposurePass>(m_device, m_swapchain->getWidth(),
                                                                m_swapchain->getHeight(), shaderDir, isMetal);
        m_colorGradingPass = std::make_unique<ColorGradingPass>(m_device, m_swapchain->getWidth(),
                                                                m_swapchain->getHeight(), shaderDir, isMetal);
        m_toneMapPass = std::make_shared<ToneMapPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal);
        m_fxaaPass = std::make_unique<FXAAPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal);

        m_mainScenePass = std::make_shared<MainScenePass>(m_device, m_swapchain, shaderDir, isMetal);
        m_particleUpdatePass = std::make_unique<ParticleUpdatePass>(m_device, shaderDir, isMetal);
        m_particleEmitterPass = std::make_unique<ParticleEmitterPass>(m_device, shaderDir, isMetal);
        m_particleCompactPass = std::make_unique<ParticleCompactPass>(m_device, shaderDir, isMetal);
        m_particleIndirectPass = std::make_unique<ParticleIndirectPass>(m_device, shaderDir, isMetal);
        m_meshCompactPass = std::make_unique<MeshCompactPass>(m_device, shaderDir, isMetal);
        m_hizMipPass = std::make_unique<HiZMipPass>(m_device, shaderDir, isMetal);
        m_occlusionCullPass = std::make_unique<OcclusionCullingPass>(m_device, shaderDir, isMetal);
        m_particleBillboardPass = std::make_unique<ParticleBillboardPass>(m_device, m_swapchain->getWidth(), m_swapchain->getHeight(), shaderDir, isMetal);
        m_copyHizDepthPass = std::make_unique<CopyHizDepthPass>(m_device, shaderDir, isMetal);

        buildRenderGraph();
    }

    void TiledDeferredRenderer::resize(uint32_t width, uint32_t height)
    {
        m_depthPrepass->resize(width, height);
        m_geometryPass->resize(width, height);
        m_tileComputePass->resize(width, height);
        m_ssaoPass->resize(width, height);
        m_tileLightPass->resize(width, height);
        m_skyboxPass->resize(width, height);
        m_deferredLightingPass->recreate(width, height);
        m_bloomEffect->resize(width, height);
        m_toneMapPass->resize(width, height);
        m_fxaaPass->resize(width, height);
        m_autoExposurePass->resize(width, height);
        m_colorGradingPass->resize(width, height);
        m_particleBillboardPass->resize(width, height);
        m_debugDrawPass->resize(width, height);

        m_renderGraph.allocateTextures(m_device, width, height);
        m_renderGraph.bindPassResources(m_renderGraph.buildResources());
    };

    void TiledDeferredRenderer::execute(rhi::RHICommandBuffer *cmd, const RenderContext &ctx, RendererSettings &settings, rhi::RHITimer *timer)
    {
        m_renderGraph.executeFrameGraph(m_compiledFrameGraph, cmd, ctx, settings, timer, m_device->getCurrentFrameIndex());
    }

    TiledDeferredRenderer::~TiledDeferredRenderer()
    {
        m_renderGraph.deleteBuffers(m_device);
        m_renderGraph.deleteTextures(m_device);
        m_device->destroyTexture(m_cubemapTexture);
        m_device->destroyTexture(m_irradianceTexture);
        m_device->destroyTexture(m_prefilterMap);
        m_device->destroyTexture(m_brdfLUT);
    };

    void TiledDeferredRenderer::buildRenderGraph()
    {

        auto drawCountId = m_renderGraph.declareBuffer({"Draw Count",
                                                        sizeof(uint32_t),
                                                        rhi::BufferDesc::Usage::Indirect,
                                                        true});
        auto drawCommandsId = m_renderGraph.declareBuffer({"Draw Command Buffer",
                                                           sizeof(DrawIndexedIndirectArgs) * Scene::s_MAX_DRAW_COMMANDS,
                                                           rhi::BufferDesc::Usage::Indirect,
                                                           true});
        auto hizTex = m_renderGraph.declareTexture({"Hiz Depth",
                                                    rhi::TextureDesc::ImageFormat::ColorR32, 0, 0, true, HIZ_MIP_COUNT});

        m_renderGraph.addPass({
            "Mesh Compact pass",
            {},
            {},
            {},
            {
                {drawCountId, rhi::ResourceState::ShaderWrite},
                {drawCommandsId, rhi::ResourceState::ShaderWrite},
            },
            [](const RGResources &resources) {

            },
            [drawCommandsId, drawCountId, hizTex, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                auto frameIdx = m_device->getCurrentFrameIndex();

                MeshCompactPushConstant pc;
                pc.objectCount = static_cast<uint32_t>(ctx.scene->instanceIds().size());

                pc.frustumCullEnabled = settings.frustumCullEnabled ? 1 : 0;
                pc.lodEnabled = settings.lodEnabled ? 1 : 0;
                pc.proj = ctx.camera->proj();
                pc.view = ctx.camera->view();
                pc.projScaleY = ctx.camera->proj()[1][1];
                pc.screenHeight = settings.viewportSize.y;
                pc.occlusionCullEnabled = settings.occlusionCullEnabled ? 1 : 0;

                m_meshCompactPass->execute(cmd, *ctx.scene, resources.getBuffer(drawCommandsId, frameIdx), resources.getBuffer(drawCountId, frameIdx), pc, resources.getTexture(hizTex));
            },
        });

        auto depth = m_renderGraph.declareTexture({"GBuffer Depth",
                                                   rhi::TextureDesc::ImageFormat::Depth32Float, 0, 0});
        auto albedo = m_renderGraph.declareTexture({"GBuffer Albedo",
                                                    rhi::TextureDesc::ImageFormat::ColorRGBA8});
        auto normal = m_renderGraph.declareTexture({"GBuffer Normal",
                                                    rhi::TextureDesc::ImageFormat::ColorRG8U});
        auto metallicRoughness = m_renderGraph.declareTexture({"GBuffer Metallic Roughness",
                                                               rhi::TextureDesc::ImageFormat::ColorRGBA8});
        auto emissive = m_renderGraph.declareTexture({"GBuffer Emissive",
                                                      rhi::TextureDesc::ImageFormat::ColorRGBA16});

        m_renderGraph.addPass({
            "Depth Prepass",
            {},
            {{depth, rhi::ResourceState::DepthWrite}},
            {
                {drawCountId, rhi::ResourceState::IndirectDraw},
                {drawCommandsId, rhi::ResourceState::IndirectDraw},
            },
            {},
            [depth, this](const RGResources &resources)
            {
                m_depthPrepass->bindResources(resources, depth);
            },
            [depth, drawCommandsId, drawCountId, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                DepthPrePassCamera depthCamera;
                depthCamera.view = ctx.camera->view();
                depthCamera.proj = ctx.camera->proj();

                auto frameIdx = m_device->getCurrentFrameIndex();

                m_depthPrepass->execute(cmd, *ctx.scene, resources.getBuffer(drawCommandsId, frameIdx), resources.getBuffer(drawCountId, frameIdx), depthCamera);
            },
        });

        m_renderGraph.addPass(
            {"Copy Depth to Hi-Z",

             {{depth, rhi::ResourceState::ShaderRead}},
             {{hizTex, rhi::ResourceState::ShaderWrite, WriteMode::Producer}},
             {},
             {},
             [](const RGResources &resources) {},
             [depth, hizTex, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
             {
                 CopyHizDepthPushConstant pc;
                 pc.textureSize = settings.viewportSize;

                 CopyHizDepthRGResource rgResource;
                 rgResource.hizTexture = resources.getTexture(hizTex);
                 rgResource.depthTexture = resources.getTexture(depth);

                 m_copyHizDepthPass->execute(cmd, pc, rgResource);
             }}

        );
        m_renderGraph.addPass(
            {"Hi-Z Mip generation",

             {},
             {{hizTex, rhi::ResourceState::ShaderRead, WriteMode::Extend}},
             {},
             {},
             [this, hizTex](const RGResources &resources)
             {
                 m_hizMipPass->bindResources(resources, hizTex);
             },
             [hizTex, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
             {
                 m_hizMipPass->execute(cmd, settings.viewportSize.x, settings.viewportSize.y, resources.getTexture(hizTex));
             }}

        );

        auto hizDrawCountId = m_renderGraph.declareBuffer({" Hi-z Draw Count",
                                                           sizeof(uint32_t),
                                                           rhi::BufferDesc::Usage::Indirect,
                                                           true});
        auto hizDrawCommandsId = m_renderGraph.declareBuffer({"Hi-z Draw Command Buffer",
                                                              sizeof(DrawIndexedIndirectArgs) * Scene::s_MAX_DRAW_COMMANDS,
                                                              rhi::BufferDesc::Usage::Indirect,
                                                              true});

        m_renderGraph.addPass(
            {"Occlusion Culling",

             {{hizTex, rhi::ResourceState::ShaderRead}},
             {},
             {{drawCountId, rhi::ResourceState::ShaderRead},
              {drawCommandsId, rhi::ResourceState::ShaderRead}

             },
             {
                 {
                     {hizDrawCountId, rhi::ResourceState::ShaderWrite},
                     {hizDrawCommandsId, rhi::ResourceState::ShaderWrite},
                 },
             },
             [](const RGResources &resources) {},
             [hizTex, hizDrawCommandsId, hizDrawCountId, drawCountId, drawCommandsId, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
             {
                 OcclusionCullPushConstant pc;
                 auto near = ctx.camera->near();
                 auto far = ctx.camera->far();
                 pc.screenHeight = m_swapchain->getHeight();
                 pc.depthScaleA = far / std::max(far - near, 0.0001f);
                 pc.view = ctx.camera->view();
                 pc.proj = ctx.camera->proj();
                 pc.maxMip = HIZ_MIP_COUNT;
                 pc.projScaleY = std::abs(ctx.camera->proj()[1][1]);
                 pc.occlusionCullEnabled = settings.occlusionCullEnabled ? 1 : 0;

                 uint32_t frameIdx = m_device->getCurrentFrameIndex();
                 OcclusionCullRGResource rgResources;
                 rgResources.hizDepthTex = resources.getTexture(hizTex);
                 rgResources.hiZDrawCommands = resources.getBuffer(hizDrawCommandsId, frameIdx);
                 rgResources.hiZDrawCount = resources.getBuffer(hizDrawCountId, frameIdx);
                 rgResources.sceneDrawCommands = resources.getBuffer(drawCommandsId, frameIdx);
                 rgResources.sceneDrawCount = resources.getBuffer(drawCountId, frameIdx);

                 //  m_occlusionCullPass->execute(cmd, pc, *ctx.scene, rgResources);
             }}

        );
        GBuffer gBufferIds{albedo, normal, metallicRoughness, emissive, depth};

        m_renderGraph.addPass({
            "GBuffer",
            {{depth, rhi::ResourceState::DepthRead}},
            {{albedo, rhi::ResourceState::RenderTarget}, {normal, rhi::ResourceState::RenderTarget}, {metallicRoughness, rhi::ResourceState::RenderTarget}, {emissive, rhi::ResourceState::RenderTarget}},
            {
                {drawCountId, rhi::ResourceState::IndirectDraw},
                {drawCommandsId, rhi::ResourceState::IndirectDraw},
            },
            {},
            [gBufferIds, this](const RGResources &resources)
            {
                m_geometryPass->bindResources(resources, gBufferIds);
            },
            [gBufferIds, drawCommandsId, drawCountId, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                GeometryCameraBuffer geometryCamera;
                geometryCamera.view = ctx.camera->view();
                geometryCamera.proj = ctx.camera->proj();

                auto frameIdx = m_device->getCurrentFrameIndex();
                m_geometryPass->execute(cmd, geometryCamera, *ctx.scene, settings.light, resources.getBuffer(drawCommandsId, frameIdx), resources.getBuffer(drawCountId, frameIdx));
            },
        });

        auto ssaoTex = m_renderGraph.declareTexture({"SSAO Texture",
                                                     rhi::TextureDesc::ImageFormat::ColorRGBA16, 0, 0, true});

        SSAOPassTextureIDs ssaoTextures{depth, normal, ssaoTex};
        m_renderGraph.addPass({
            "SSAO Pass",
            {{depth, rhi::ResourceState::ShaderRead}, {normal, rhi::ResourceState::ShaderRead}},
            {{ssaoTex, rhi::ResourceState::ShaderWrite}},
            {},
            {},
            [ssaoTextures, this](const RGResources &resources)
            {
                m_ssaoPass->bindResources(resources, ssaoTextures);
            },
            [ssaoTextures, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                SSAOPushConstant ssaoPc;
                ssaoPc.invProj = glm::inverse(ctx.camera->proj());
                ssaoPc.view = ctx.camera->view();
                ssaoPc.proj = ctx.camera->proj();
                ssaoPc.textureSize = settings.viewportSize;
                ssaoPc.totalSamples = static_cast<uint>(ctx.ssaoSamples.samples.size());
                ssaoPc.radius = settings.ssao.radius;
                // m_ssaoPass->execute(cmd, ssaoPc, resources, ssaoTextures.ssaoTex, ctx.ssaoSamples.samples, settings.ssao.depthSigma);
            },
        });

        auto pointLightTex = m_renderGraph.declareTexture({"Tile Light Texture",
                                                           rhi::TextureDesc::ImageFormat::ColorRGBA16});

        TileLightShadingTextureIDs tileLightTextures{depth, normal, pointLightTex};
        m_renderGraph.addPass({
            "Tile Light Pass",
            {{depth, rhi::ResourceState::ShaderRead}, {normal, rhi::ResourceState::ShaderRead}},
            {{pointLightTex, rhi::ResourceState::RenderTarget}},
            {},
            {},
            [tileLightTextures, this](const RGResources &resources)
            {
                m_tileComputePass->bindResource(resources, tileLightTextures.gDepth);
                m_tileLightPass->bindResources(resources, tileLightTextures, m_tileComputePass->getFrameResources());
            },
            [tileLightTextures, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                TiledCameraUBO computeUBO;
                computeUBO.farPlane = ctx.camera->far();
                computeUBO.nearPlane = ctx.camera->near();
                computeUBO.screenSize = settings.viewportSize;
                computeUBO.invProj = glm::inverse(ctx.camera->proj());
                computeUBO.view = ctx.camera->view();
                computeUBO.totalLightCount = static_cast<uint>(settings.light.pointLights.size());

                m_tileComputePass->execute(cmd, settings.light, computeUBO);

                TiledLightPassUBO lightPassUBO;

                lightPassUBO.invViewProj = ctx.camera->invViewProj();
                lightPassUBO.view = ctx.camera->view();
                lightPassUBO.numTilesX = static_cast<uint32_t>(
                    std::ceil(float(settings.viewportSize.x) / 16.0f));
                lightPassUBO.screenSize = settings.viewportSize;
                lightPassUBO.showHeatMap = settings.selectedDebugMode == DebugMode::HeatMap ? 1 : 0;
                m_tileLightPass->execute(cmd, lightPassUBO);
            },
        });

        auto skyboxTex = m_renderGraph.declareTexture({"Skybox",
                                                       rhi::TextureDesc::ImageFormat::ColorRGBA16});

        SkyboxTextures skyboxTextures{m_cubemapTexture, skyboxTex};
        m_renderGraph.addPass({
            "Skybox",
            {},
            {{skyboxTex, rhi::ResourceState::RenderTarget}},
            {},
            {},
            [skyboxTextures, this](const RGResources &resources)
            {
                m_skyboxPass->bindResources(resources, skyboxTextures);
            },
            [this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                SkyboxPassUBO skyboxUbo;
                glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(ctx.camera->view()));
                skyboxUbo.viewProj = glm::inverse(ctx.camera->proj() * viewNoTranslation);
                skyboxUbo.screenSize = settings.viewportSize;
                m_skyboxPass->execute(cmd, skyboxUbo);
            },
        });

        std::vector<RGTextureID> cascadeTextures;
        for (int i = 0; i < CascadeShadowMapPass::CASCADE_COUNT; i++)
        {
            cascadeTextures.push_back(m_renderGraph.declareTexture({
                "Shadow map" + std::to_string(i + 1),
                rhi::TextureDesc::ImageFormat::Depth32Float,
                ShadowPass::c_ShadowResolution,
                ShadowPass::c_ShadowResolution,
            }));
        }

        std::vector<RGResourceAccess> shadowMapWrites;

        for (auto &id : cascadeTextures)
        {
            shadowMapWrites.push_back({id, rhi::ResourceState::DepthWrite});
        }
        m_renderGraph.addPass({
            "Shadow map",
            {},
            {shadowMapWrites},
            {
                {drawCountId, rhi::ResourceState::IndirectDraw},
                {drawCommandsId, rhi::ResourceState::IndirectDraw},
            },
            {},
            [cascadeTextures, this](const RGResources &resources)
            {
                m_csmPass->bindResources(resources, cascadeTextures);
            },
            [this, drawCommandsId, drawCountId](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                CascadeShadowContext shadowCtx;
                shadowCtx.aspect = settings.viewportSize.x / settings.viewportSize.y;
                shadowCtx.cameraFar = ctx.CAMERA_FAR;
                shadowCtx.cameraNear = ctx.CAMERA_NEAR;
                shadowCtx.fov = ctx.camera->fov();
                shadowCtx.lambda = settings.shadow.lambda;
                shadowCtx.cameraView = ctx.camera->view();
                shadowCtx.lightView = settings.light.lightCamera.view();
                auto frameIdx = m_device->getCurrentFrameIndex();
                m_csmPass->execute(cmd, *ctx.scene, shadowCtx, resources.getBuffer(drawCommandsId, frameIdx), resources.getBuffer(drawCountId, frameIdx));
            },
        });

        auto lightShadedTex = m_renderGraph.declareTexture({"HDR Light Output",
                                                            rhi::TextureDesc::ImageFormat::ColorRGBA16});

        DeferredLightingTextureIDs deferredLightIds{
            gBufferIds,
            pointLightTex,
            skyboxTex,
            cascadeTextures,
            ssaoTex,
            m_irradianceTexture,
            m_brdfLUT,
            m_prefilterMap,
            lightShadedTex};

        std::vector<RGResourceAccess> deferredLightingReads{
            {gBufferIds.albedo, rhi::ResourceState::ShaderRead},
            {gBufferIds.depth, rhi::ResourceState::ShaderRead},
            {gBufferIds.normal, rhi::ResourceState::ShaderRead},
            {gBufferIds.material, rhi::ResourceState::ShaderRead},
            {gBufferIds.emissive, rhi::ResourceState::ShaderRead},
            {skyboxTex, rhi::ResourceState::ShaderRead},
            {pointLightTex, rhi::ResourceState::ShaderRead},
            {ssaoTex, rhi::ResourceState::ShaderRead}};
        for (int i = 0; i < cascadeTextures.size(); i++)
        {
            deferredLightingReads.push_back({cascadeTextures[i], rhi::ResourceState::ShaderRead});
        }

        m_renderGraph.addPass({
            "Deferred Lighting",
            deferredLightingReads,
            {{lightShadedTex, rhi::ResourceState::RenderTarget}},
            {},
            {},
            [deferredLightIds, this](const RGResources &resources)
            {
                m_deferredLightingPass->bindResources(resources, deferredLightIds);
            },
            [this, lightShadedTex](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
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
                frameData.invViewProj = ctx.camera->invViewProj();
                frameData.view = ctx.camera->view();
                frameData.cameraPosition = glm::vec4(ctx.camera->getEye(), 1.0f);

                frameData.lightPosition = glm::vec4(settings.light.lightCamera.getEye(), 1.0f);
                frameData.lightColor = glm::vec4(settings.light.lightColor, 1.0f);
                frameData.shadowBias = settings.shadow.bias;
                frameData.shadowNormalBias = settings.shadow.normalBias;
                frameData.showCascadeColors = settings.shadow.showCascadeColors ? 1.0f : 0.0f;
                frameData.debugMode = static_cast<float>(settings.selectedDebugMode);
                frameData.lightMode = static_cast<float>(settings.selectedLightMode);
                frameData.roughness = settings.light.roughness;
                // for (int i = 0; i < settings.light.pointLights.size(); i++)
                // {
                //     frameData.pointLights[i] = settings.light.pointLights[i];
                // }

                m_deferredLightingPass->execute(cmd, frameData);
                m_currentSceneTextureID = lightShadedTex;
            },
        });

        auto particleBuffer = m_renderGraph.declareBuffer({"Particle Buffer",
                                                           sizeof(ParticleDesc) * ParticleUpdatePass::s_MAX_PARTICLE_COUNT,
                                                           rhi::BufferDesc::Usage::Storage});
        auto deadListBuffer = m_renderGraph.declareBuffer({"Dead List Buffer",
                                                           (sizeof(uint32_t) * ParticleUpdatePass::s_MAX_PARTICLE_COUNT) + sizeof(uint32_t),
                                                           rhi::BufferDesc::Usage::Storage});
        auto aliveListBuffer = m_renderGraph.declareBuffer({"Alive List Buffer",
                                                            (sizeof(uint32_t) * ParticleUpdatePass::s_MAX_PARTICLE_COUNT),
                                                            rhi::BufferDesc::Usage::Storage});
        auto aliveCountBuffer = m_renderGraph.declareBuffer({"Alive Count Buffer",
                                                             (sizeof(uint32_t)),
                                                             rhi::BufferDesc::Usage::Storage | rhi::BufferDesc::Usage::TransferDst});
        auto emitterBuffer = m_renderGraph.declareBuffer({"Emitter Buffer",
                                                          (sizeof(EmitterDesc) * ParticleEmitterSystem::s_MAX_EMITTERS),
                                                          rhi::BufferDesc::Usage::Storage | rhi::BufferDesc::Usage::TransferDst});

        auto indirectDrawBuffer = m_renderGraph.declareBuffer({"Indirect Draw Buffer",
                                                               sizeof(rhi::DrawIndirectArgs),
                                                               rhi::BufferDesc::Usage::Indirect});
        auto indirectDispatchBuffer = m_renderGraph.declareBuffer({"Indirect Dispatch Buffer",
                                                                   sizeof(rhi::DrawIndirectArgs),
                                                                   rhi::BufferDesc::Usage::Indirect});
        m_renderGraph.addPass({
            "Particle Emitter",
            {},
            {},
            {},
            {{emitterBuffer, rhi::ResourceState::ShaderWrite},
             {deadListBuffer, rhi::ResourceState::ShaderWrite}},
            [particleBuffer, deadListBuffer, emitterBuffer, this](const RGResources &resources)
            {
                m_particleEmitterPass->bindResources(resources, {particleBuffer,
                                                                 deadListBuffer,
                                                                 emitterBuffer});
            },
            [particleBuffer, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                ParticleEmitterPushConstant pc;
                pc.emitterCount = m_emitterSystem.getEmitterCount();
                pc.frameIndex = m_device->getCurrentFrameIndex();
                pc.frameTime = ctx.deltaTime;
                pc.maxParticles = ParticleUpdatePass::s_MAX_PARTICLE_COUNT;

                m_particleEmitterPass->execute(cmd, pc);
            },
        });
        m_renderGraph.addPass({
            "Particle Compact Pass",
            {},
            {},
            {{deadListBuffer, rhi::ResourceState::ShaderWrite}},

            {{aliveListBuffer, rhi::ResourceState::ShaderWrite},
             {aliveCountBuffer, rhi::ResourceState::ShaderWrite}},
            [particleBuffer, aliveListBuffer, aliveCountBuffer, this](const RGResources &resources)
            {
                m_particleCompactPass->bindResources(resources, {particleBuffer,
                                                                 aliveListBuffer,
                                                                 aliveCountBuffer});
            },
            [aliveCountBuffer, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                cmd->fillBuffer(resources.getBuffer(aliveCountBuffer), 0, sizeof(uint32_t), 0);

                rhi::BufferBarrier barrier;
                barrier.buffer = resources.getBuffer(aliveCountBuffer);
                barrier.before = rhi::ResourceState::CopyDst;
                barrier.after = rhi::ResourceState::ShaderWrite;
                cmd->bufferBarrier(barrier);

                ParticleCompactPushConstant pc;
                pc.particleCount = ParticleUpdatePass::s_MAX_PARTICLE_COUNT;

                m_particleCompactPass->execute(cmd, pc);
            },
        });

        m_renderGraph.addPass({
            "Particle Indirect Copy",
            {},
            {},
            {{aliveCountBuffer, rhi::ResourceState::ShaderRead}},
            {
                {indirectDrawBuffer, rhi::ResourceState::ShaderWrite},
                {indirectDispatchBuffer, rhi::ResourceState::ShaderWrite},
            },
            [aliveCountBuffer, indirectDrawBuffer, indirectDispatchBuffer, this](const RGResources &resources)
            {
                m_particleIndirectPass->bindResources(resources, {aliveCountBuffer,
                                                                  indirectDrawBuffer,
                                                                  indirectDispatchBuffer});
            },
            [this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                m_particleIndirectPass->execute(cmd);
            },
        });
        m_renderGraph.addPass({
            "Particle Update",
            {},
            {},
            {
                {emitterBuffer, rhi::ResourceState::ShaderWrite},
                {deadListBuffer, rhi::ResourceState::ShaderWrite},
                {aliveCountBuffer, rhi::ResourceState::ShaderRead},
                {aliveListBuffer, rhi::ResourceState::ShaderRead},
                {indirectDispatchBuffer, rhi::ResourceState::ShaderRead},
            },
            {{particleBuffer, rhi::ResourceState::ShaderWrite}},
            [particleBuffer, deadListBuffer, aliveCountBuffer, aliveListBuffer, emitterBuffer, this](const RGResources &resources)
            {
                m_particleUpdatePass->bindResources(resources, {particleBuffer,
                                                                aliveListBuffer,
                                                                aliveCountBuffer,
                                                                deadListBuffer,
                                                                emitterBuffer});
            },
            [indirectDispatchBuffer, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                ParticlePushConstant pc;
                pc.dt = float(ctx.deltaTime);
                pc.particleCount = ParticleUpdatePass::s_MAX_PARTICLE_COUNT;
                pc.gravity = -9.8f;

                m_particleUpdatePass->execute(cmd, pc, resources.getBuffer(indirectDispatchBuffer));
            },
        });

        auto particleTexture = m_renderGraph.declareTexture({"Particle Texture",
                                                             rhi::TextureDesc::ImageFormat::ColorRGBA16});

        m_renderGraph.addPass(
            {"Copy Light to Particle",

             {{lightShadedTex, rhi::ResourceState::CopySrc}},
             {{particleTexture, rhi::ResourceState::CopyDst, WriteMode::Producer}},
             {},
             {},
             [](const RGResources &resources) {},
             [particleTexture, lightShadedTex, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
             {
                 cmd->copyTextureToTexture(resources.getTexture(lightShadedTex), resources.getTexture(particleTexture));
             }}

        );
        m_renderGraph.addPass({
            "Particle Billboard",
            {{lightShadedTex, rhi::ResourceState::ShaderRead}},
            {{particleTexture, rhi::ResourceState::RenderTarget, WriteMode::Extend}},
            {{particleBuffer, rhi::ResourceState::ShaderRead},
             {aliveListBuffer, rhi::ResourceState::ShaderWrite},
             {indirectDrawBuffer, rhi::ResourceState::ShaderRead}},
            {},
            [particleTexture, particleBuffer, aliveListBuffer, this](const RGResources &resources)
            {
                m_particleBillboardPass->bindResources(resources, {particleBuffer,
                                                                   aliveListBuffer,
                                                                   particleTexture});
            },
            [indirectDrawBuffer, particleTexture, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                glm::mat4 inverseView = glm::inverse(ctx.camera->view());

                ParticleBillboardUBO ubo;
                ubo.view = ctx.camera->view();
                ubo.proj = ctx.camera->proj();
                ubo.right = inverseView[0];
                ubo.up = inverseView[1];

                m_particleBillboardPass->execute(cmd, ubo, resources.getBuffer(indirectDrawBuffer));
                m_currentSceneTextureID = particleTexture;
            },
        });

        auto bloomTexture = m_renderGraph.declareTexture({"Bloom Texture",
                                                          rhi::TextureDesc::ImageFormat::ColorRGBA16, 0, 0, true});

        m_renderGraph.addPass({
            "Bloom",
            {{particleTexture, rhi::ResourceState::ShaderRead}},
            {{bloomTexture, rhi::ResourceState::ShaderWrite}},
            {{particleBuffer, rhi::ResourceState::ShaderRead}},
            {},
            [](const RGResources &resources) {},
            [bloomTexture, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                if (!settings.bloom.enable)
                    return;

                m_bloomEffect->execute(cmd, {resources.getTexture(m_currentSceneTextureID), resources.getTexture(bloomTexture)}, settings.bloom);
                m_currentSceneTextureID = bloomTexture;
            },
        });

        auto readbackBuffer = m_renderGraph.declareBuffer({
            "Auto Exposure Readback buffer",
            16,
            rhi::BufferDesc::Usage::TransferDst,
        });

        m_renderGraph.addPass({
            "Auto exposure pass",
            {{bloomTexture, rhi::ResourceState::ShaderRead}},
            {},
            {},
            {{readbackBuffer, rhi::ResourceState::ShaderWrite}},
            [](const RGResources &resources) {},
            [this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                if (!settings.tonemap.autoExposure)
                    return;

                AutoExposurePushConstant autoExposurePc;
                autoExposurePc.inputTextureSize = settings.viewportSize;

                settings.tonemap.exposure = m_autoExposurePass->execute(cmd, autoExposurePc, resources.getTexture(m_currentSceneTextureID), settings.tonemap, ctx.deltaTime);
            },
        });

        auto colorGradedTexture = m_renderGraph.declareTexture({"Color Grade",
                                                                rhi::TextureDesc::ImageFormat::ColorRGBA16, 0, 0, true});

        m_renderGraph.addPass({
            "Color Grading",
            {{bloomTexture, rhi::ResourceState::ShaderRead}},
            {{colorGradedTexture, rhi::ResourceState::ShaderWrite}},
            {{readbackBuffer, rhi::ResourceState::ShaderRead}},
            {},
            [](const RGResources &resources) {},
            [colorGradedTexture, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                if (!settings.colorGrading.enable)
                    return;
                ColorGradingPushConstant pc;
                pc.gain = glm::vec4(settings.colorGrading.gain, 1.0);
                pc.lift = glm::vec4(settings.colorGrading.lift, 1.0);
                pc.gamma = glm::vec4(settings.colorGrading.gamma, 1.0);
                pc.textureSize = settings.viewportSize;
                m_colorGradingPass->execute(cmd, pc, {resources.getTexture(m_currentSceneTextureID), resources.getTexture(colorGradedTexture)});
                m_currentSceneTextureID = colorGradedTexture;
            },
        });

        auto tonemapTexture = m_renderGraph.declareTexture({"ToneMap",
                                                            rhi::TextureDesc::ImageFormat::ColorRGBA8});

        m_renderGraph.addPass({
            "Tone Map",
            {{colorGradedTexture, rhi::ResourceState::ShaderRead}},
            {{tonemapTexture, rhi::ResourceState::RenderTarget}},
            {},
            {},
            [tonemapTexture, this](const RGResources &resources)
            {
                m_toneMapPass->bindResource(resources, tonemapTexture);
            },
            [tonemapTexture, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                ToneMapPassUBO toneMapUBO;
                toneMapUBO.exposure = settings.tonemap.exposure;
                toneMapUBO.mode = static_cast<uint>(settings.tonemap.mode);
                m_toneMapPass->execute(cmd, toneMapUBO, resources.getTexture(m_currentSceneTextureID));
                m_currentSceneTextureID = tonemapTexture;
            },
        });

        auto fxaaTexture = m_renderGraph.declareTexture({"FXAA",
                                                         rhi::TextureDesc::ImageFormat::ColorRGBA8, 0, 0, true});

        m_renderGraph.addPass({
            "FXAA",
            {{tonemapTexture, rhi::ResourceState::ShaderRead}},
            {{fxaaTexture, rhi::ResourceState::ShaderWrite, WriteMode::Producer}},
            {},
            {},
            [](const RGResources &resources) {

            },
            [fxaaTexture, this](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                FXAAPushConstant fxaaPc;
                fxaaPc.textureSize = settings.viewportSize;

                m_fxaaPass->execute(cmd, fxaaPc, {resources.getTexture(m_currentSceneTextureID), resources.getTexture(fxaaTexture)});
                m_currentSceneTextureID = fxaaTexture;
            },
        });

        auto debugTexture = m_renderGraph.declareTexture({"Debug Draw",
                                                          rhi::TextureDesc::ImageFormat::ColorRGBA8, 0, 0, false});
        m_renderGraph.addPass({
            "Copy FXAA Pass",
            {{fxaaTexture, rhi::ResourceState::CopySrc}},
            {{debugTexture, rhi::ResourceState::CopyDst}},
            {},
            {},
            [](const RGResources &resources) {

            },
            [this, fxaaTexture, debugTexture](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                cmd->copyTextureToTexture(resources.getTexture(fxaaTexture), resources.getTexture(debugTexture));
            },
        });
        m_renderGraph.addPass({
            "Debug Draw Pass",
            {},
            {{debugTexture, rhi::ResourceState::RenderTarget, WriteMode::Extend}},
            {},
            {},
            [this, debugTexture](const RGResources &resources)
            {
                m_debugDrawPass->bindResources(resources, debugTexture);
            },
            [this, debugTexture](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                if (settings.debugDrawPicking)
                {
                    auto &lastPick = ctx.scene->lastPick();
                    m_debugDrawPass->drawRay(lastPick.ray.origin, lastPick.ray.dir, lastPick.ray.tMax, DebugColor::Ray);

                    if (settings.debugDrawTestedBoxes)
                    {
                        for (auto &h : lastPick.tested)
                        {
                            auto instance = ctx.scene->meshManager->getMeshInstance(h);
                            if (!instance)
                            {
                                continue;
                            }
                            m_debugDrawPass->drawAABB(instance->worldAABBMin, instance->worldAABBMax, DebugColor::Tested);
                        }
                    }

                    for (auto &h : lastPick.hit)
                    {
                        auto instance = ctx.scene->meshManager->getMeshInstance(h);
                        if (!instance)
                        {
                            continue;
                        }
                        m_debugDrawPass->drawAABB(instance->worldAABBMin, instance->worldAABBMax, DebugColor::Hit);
                    }

                    if (lastPick.best.has_value())
                    {
                        auto instance = ctx.scene->meshManager->getMeshInstance(lastPick.best.value());

                        if (instance)
                        {
                            m_debugDrawPass->drawAABB(instance->worldAABBMin, instance->worldAABBMax, DebugColor::Best);
                        }
                    }
                }
                auto selectedHandle = ctx.scene->selectedInstance();

                if (selectedHandle.has_value() && selectedHandle.value().isValid())
                {
                    auto instance = ctx.scene->meshManager->getMeshInstance(selectedHandle.value());
                    m_debugDrawPass->drawAABB(instance->worldAABBMin, instance->worldAABBMax, DebugColor::Selected);
                }
                m_debugDrawPass->execute(cmd, ctx.camera->viewProj());
                m_currentSceneTextureID = debugTexture;
            },
        });
        m_renderGraph.addPass({
            "Final Scene",
            {{debugTexture, rhi::ResourceState::ShaderRead}},
            {},
            {},
            {},
            [](const RGResources &resources) {

            },
            [this, emitterBuffer, debugTexture](rhi::RHICommandBuffer *cmd, const RGResources &resources, const RenderContext &ctx, RendererSettings &settings)
            {
                rhi::RHIRenderPassDesc rpDesc{};
                rpDesc.clearColor[0] = 0.0f;
                rpDesc.clearColor[1] = 0.0f;
                rpDesc.clearColor[2] = 0.0f;
                rpDesc.clearColor[3] = 1.0f;
                rpDesc.clearDepth = 1.0f;
                rpDesc.hasDepth = true;

                m_mainScenePass->execute(cmd, rpDesc, settings, resources.getTexture(m_currentSceneTextureID), m_renderGraph, m_emitterSystem, resources.getBuffer(emitterBuffer), ctx);
            },
        });

        m_renderGraph.compile();
        m_renderGraph.allocateTextures(m_device, m_swapchain->getWidth(), m_swapchain->getHeight());
        m_renderGraph.allocateBuffers(m_device);
        auto resources = m_renderGraph.buildResources();
        m_renderGraph.bindPassResources(resources);
        // m_particleEmitterPass->uploadInitialEmitter(resources, emitterBuffer);
        m_particleUpdatePass->uploadDeadList(resources, deadListBuffer);
        m_compiledFrameGraph = m_renderGraph.compileFrameGraph();

        for (int i = 0; i < g_MAX_FRAMES_IN_FLIGHT; i++)
        {
            uint32_t initialCount = 0;
            resources.getBuffer(drawCountId, i)->upload(&initialCount, sizeof(uint32_t));
            resources.getBuffer(hizDrawCountId, i)->upload(&initialCount, sizeof(uint32_t));
        }
    }
} // namespace nitro::renderer

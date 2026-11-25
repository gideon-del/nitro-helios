#include "nitro-studio/application.h"
#include "nitro-studio/device-factory.h"
#include <random>
#include "nitro-renderer/render-pipeline/deferred-pipeline.h"
#include <string>
#include <ranges>
#include "nitro-renderer/utils.h"
namespace nitro::studio
{

    static void addRandomSpheres(uint32_t count, float areaSize, renderer::Scene &scene, renderer::GPUMeshHandle mesh, renderer::GPUMaterialHandle material)
    {
        std::mt19937 rng(50); // fixed seed
        std::uniform_real_distribution<float> pos(-areaSize, areaSize);

        for (uint32_t i = 0; i < count; i++)
        {

            auto transformation = geometry::MeshTransformation(glm::translate(glm::mat4(1.0f), glm::vec3(pos(rng), 10.0f, pos(rng))));

            scene.addMeshEntity(mesh, material, transformation, "Sphere");
        }
    }

    static GLFWwindow *createWindow(int width, int height, const char *title)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow *window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window)
            throw std::runtime_error("Failed to create window");
        return window;
    }

    Application::Application()
        : m_window(createWindow(800, 600, "Nitro Studio")),
          m_backend(defaultBackend()),
          m_device(createDevice(m_backend, m_window.get())),
          m_renderer(*m_device.get(), std::string(SHADER_DIR), m_backend == Backend::Metal),
          m_swapchain(m_device->createSwapchain(nullptr)),
          m_assetManager(std::make_shared<assets::AssetManager>()),
          m_entityStore(std::make_shared<renderer::EntityStore>()),
          m_gpuResourceCache(std::make_shared<renderer::GPUResourceCache>(m_device)),
          m_meshManager(std::make_shared<renderer::MeshManager>(m_device, m_assetManager, m_entityStore)),
          m_materialManager(std::make_shared<renderer::MaterialManager>(m_device, m_assetManager, m_gpuResourceCache)),
          m_lightManager(std::make_shared<renderer::LightManager>(m_device, m_entityStore)),
          m_scene(m_device, m_meshManager, m_materialManager, m_assetManager, m_entityStore, m_lightManager)

    {

        m_view = m_renderer.createView({800, 600}, std::make_unique<renderer::DeferredPipeline>());
        m_viewportExtent = {800, 600};
        int w, h;
        glfwGetFramebufferSize(m_window.get(), &w, &h);

        m_swapchainExtent = {(uint32_t)w, (uint32_t)h};
        m_frameClock.time = glfwGetTime();

        auto sphereMesh = m_meshManager->addMesh(geometry::MeshGenerator::createUVSphere(5, 10, 100));

        auto material = std::make_unique<assets::Material>();
        material->parameters.roughness = 0.5f;
        material->parameters.metallic = 0.0f;

        assets::MaterialAssetHandle sphereMaterial = m_assetManager->registerMaterial(std::move(material), "Sphere Material");

        addRandomSpheres(100, 400, m_scene, sphereMesh, m_materialManager->addMaterial(sphereMaterial));

        m_meshManager->buildMegaBuffers();
        m_materialManager->buildMegaMaterialBuffer();
        m_scene.buildSceneInstanceId();
        m_editorCamera.setFlipY(m_backend == Backend::Vulkan);
        m_editorCamera.setLens(60.0f, 0.1f, 1000.0f);

        scanEnvironments();

        if (!m_hdrFiles.empty())
        {
            bakeEnvironment(m_hdrFiles[m_selectedHdr]);
        }
        m_scene.loadGltfScene("./assets/gltf/DamagedHelmet/DamagedHelmet.gltf", m_device);
    }

    renderer::CameraData toCameraData(const geometry::OrbitalCamera &camera)
    {
        renderer::CameraData cameraData;

        cameraData.farPlane = camera.far();
        cameraData.nearPlane = camera.near();
        cameraData.invViewProj = camera.invViewProj();
        cameraData.proj = camera.proj();
        cameraData.view = camera.view();
        cameraData.viewProj = camera.viewProj();
        cameraData.position = camera.getEye();
        cameraData.aspect = float(camera.width()) / float(camera.height());
        cameraData.fovY = camera.fov();

        return cameraData;
    }
    Application::~Application()
    {
        m_device->waitIdle();
        destroyEnvironment();
    }

    void Application::run()
    {

        while (!glfwWindowShouldClose(m_window.get()))
        {
            glfwPollEvents();

            m_frameClock.tick(glfwGetTime());
            m_device->beginImGuiFrame();
            handleCameraInput();

            handleResize();
            renderFrame();
        }
    }

    void Application::flush()
    {
        m_meshManager->flusDirtyMeshInstances();
        m_materialManager->flush();
        m_lightManager->flush();
        m_scene.flush();

        m_sceneData.indexBuffer = m_meshManager->getIndexMegaBuffer()->getHandle();
        m_sceneData.vertexBuffer = m_meshManager->getVertexMegaBuffer()->getHandle();
        m_sceneData.meshBuffer = m_meshManager->descriptorBuffer()->getHandle();
        m_sceneData.meshInstanceBuffer = m_meshManager->instanceBuffer()->getHandle();
        m_sceneData.meshInstanceCount = m_meshManager->instanceCount();

        m_sceneData.materialBuffer = m_materialManager->getMaterialBuffer()->getHandle();
        m_sceneData.materialTextureLayout = m_materialManager->textureLayout();
        m_sceneData.materialTextureSet = m_materialManager->textureSet();

        m_sceneData.sceneInstanceIdBuffer = m_scene.sceneInstanceIdBuffer()->getHandle();

        m_sceneData.lightBuffer = m_lightManager->getPointLightBuffer()->getHandle();

        m_sceneData.lightCount = m_lightManager->pointLightCount();
        m_sceneData.directionalLight = m_sun;

        if (m_pendingEnvironment)
        {
            m_device->waitIdle();
            destroyEnvironment();
            bakeEnvironment(m_hdrFiles[m_selectedHdr]);
            m_pendingEnvironment = false;
        }
        m_sceneData.environment = m_environment;
    }

    void Application::handleCameraInput()
    {
        const ImGuiIO &io = ImGui::GetIO();

        bool dragging = io.MouseDown[ImGuiMouseButton_Left] || io.MouseDown[ImGuiMouseButton_Right];
        if (m_viewportHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
                                  ImGui::IsMouseClicked(ImGuiMouseButton_Right)))
            m_cameraDragging = true;
        if (!dragging)
            m_cameraDragging = false;

        CameraInput input{};
        input.enabled = m_viewportHovered || m_cameraDragging;
        input.mouseDelta = {io.MouseDelta.x, io.MouseDelta.y};
        input.orbit = m_cameraDragging && io.MouseDown[ImGuiMouseButton_Left];
        input.scroll = m_viewportHovered ? io.MouseWheel : 0.0f;
        if (m_viewportFocused)
        {
            input.move.y = ImGui::IsKeyDown(ImGuiKey_W) - ImGui::IsKeyDown(ImGuiKey_S);
            input.move.x = ImGui::IsKeyDown(ImGuiKey_D) - ImGui::IsKeyDown(ImGuiKey_A);
        }
        m_cameraController.update(m_editorCamera, input, m_frameClock.dt);
    }

    void Application::scanEnvironments()
    {
        m_hdrFiles.clear();

        for (const auto &entry : std::filesystem::directory_iterator("./assets/environments"))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".hdr")
                m_hdrFiles.push_back(entry.path());
        }
        std::ranges::sort(m_hdrFiles);
    }

    void Application::bakeEnvironment(const std::filesystem::path &path)
    {
        auto hdriTexture = renderer::loadHDRImage(m_device, path.c_str());
        auto cubemapTexture = renderer::createCubeMap(m_device, hdriTexture, 512, std::string(SHADER_DIR), m_backend == Backend::Metal);
        auto irradianceTexture = renderer::generateIrradianceMap(m_device, cubemapTexture, 64, std::string(SHADER_DIR), m_backend == Backend::Metal);
        auto prefilteredTexture = renderer::generatePrefliteredMap(m_device, cubemapTexture, 512, std::string(SHADER_DIR), m_backend == Backend::Metal, 5);
        auto brdfLUT = renderer::generateBrdfLUT(m_device, 512, std::string(SHADER_DIR), m_backend == Backend::Metal);

        m_device->waitIdle();
        m_environment.environment = cubemapTexture->getHandle();
        m_environment.irradiance = irradianceTexture->getHandle();
        m_environment.prefiltered = prefilteredTexture->getHandle();
        m_environment.brdfLut = brdfLUT->getHandle();
        m_environment.prefilterMipCount = 5;

        m_device->destroy(hdriTexture->getHandle());
    }

    void Application::destroyEnvironment()
    {
        m_device->destroy(m_environment.environment);
        m_device->destroy(m_environment.irradiance);
        m_device->destroy(m_environment.prefiltered);
        m_device->destroy(m_environment.brdfLut);
    }

    void Application::handleResize()
    {

        if (m_pendingViewResize)
        {
            auto e = *m_pendingViewResize;
            m_renderer.resize(m_view, e);
            m_editorCamera.setViewPort(e.width, e.height);
            m_pendingViewResize = std::nullopt;
        }
        int w, h;
        glfwGetFramebufferSize(m_window.get(), &w, &h);

        if (w == 0 || h == 0)
            return;
        Extent2D extent{uint32_t(w), uint32_t(h)};

        if (extent == m_swapchainExtent)
            return;

        m_device->waitIdle();
        m_swapchain->resize(extent.width, extent.height);
        m_swapchainExtent = extent;
    }

    void Application::renderFrame()
    {

        auto *cmd = m_device->beginFrame();

        flush();
        frame::FrameInfo frameInfo{
            .cmd = *cmd,
            .frameIndex = m_device->getCurrentFrameIndex(),
            .frameNumber = m_frameClock.frameNumber,

        };

        renderer::RenderRequest request{};

        request.sceneData = &m_sceneData;
        request.camera = toCameraData(m_editorCamera);
        request.pipelineSettings = m_pipelineSettings;
        request.postSettings = m_postSettings;
        auto sceneTexture = m_renderer.render(m_view, request, frameInfo);

        rhi::RHIRenderPassDesc rpDesc{};
        rpDesc.clearColor[0] = 0.0f;
        rpDesc.clearColor[1] = 1.0f;
        rpDesc.clearColor[2] = 1.0f;
        rpDesc.clearColor[3] = 1.0f;
        rpDesc.clearDepth = 1.0f;
        rpDesc.hasDepth = true;

        cmd->beginRenderPass(rpDesc);

        ImGui::DockSpaceOverViewport();
        ImGui::Begin("Viewport");
        // ImGui::Text("dt: %.3f ms", m_frameClock.dt * 1000.0f);

        ImVec2 size = ImGui::GetContentRegionAvail();

        ImGui::Image((ImTextureID)m_device->getImGuiTextureRef(m_device->resolve(sceneTexture)), size);
        m_viewportHovered = ImGui::IsItemHovered();
        m_viewportFocused = ImGui::IsWindowFocused();

        float dpi = ImGui::GetWindowViewport()->DpiScale;
        Extent2D pixels{uint32_t(size.x * dpi), uint32_t(size.y * dpi)};
        if (pixels != m_viewportExtent && pixels.width >= 8 && pixels.height >= 8)
        {
            m_pendingViewResize = pixels;
            m_viewportExtent = pixels;
        }

        ImGui::End();

        ImGui::Begin("Settings");

        static constexpr const char *kModes[] = {
            "Lit",
            "Depth",
            "Albedo",
            "Normal",
            "Material",
            "Emissive",
            "World Position",
            "GGX (D)",
            "Fresnel (F)",
            "Geometry (G)",
            "Ambient",
            "Direct",
            "DiffuseIBL",
            "SpeculaIBL",
            "Shadow Cascade",
        };
        static_assert(IM_ARRAYSIZE(kModes) == static_cast<int>(renderer::PipelineSettings::DebugMode::Cascade) + 1);

        int mode = static_cast<int>(m_pipelineSettings.debugMode);
        if (ImGui::Combo("View", &mode, kModes, IM_ARRAYSIZE(kModes)))
            m_pipelineSettings.debugMode = static_cast<renderer::PipelineSettings::DebugMode>(mode);

        ImGui::SeparatorText("Sun");
        if (ImGui::DragFloat3("Direction", &m_sun.direction.x, 0.01f, -1.0f, 1.0f))
            if (glm::length(m_sun.direction) > 1e-4f)
                m_sun.direction = glm::normalize(m_sun.direction);
        ImGui::ColorEdit3("Color", &m_sun.color.x);
        ImGui::DragFloat("Intensity", &m_sun.intensity, 0.05f, 0.0f, 20.0f);

        // ImGui::SeparatorText("Environment");
        // ImGui::ColorEdit3("Ambient", &m_environment.ambientColor.x);
        // ImGui::DragFloat("Ambient Intensity", &m_environment.ambientIntensity, 0.005f, 0.0f, 2.0f);
        // ImGui::ColorEdit3("Sky", &m_environment.skyColor.x);

        ImGui::SeparatorText("Tonemap");
        ImGui::DragFloat("Exposure", &m_postSettings.tonemap.exposure, 0.05f, 0.0f, 20.0f);

        static constexpr const char *kTonemapModes[] = {
            "Linear",
            "Reinhard",
            "ACES",
        };

        int tonemapMode = static_cast<int>(m_postSettings.tonemap.mode);
        if (ImGui::Combo("Mode", &tonemapMode, kTonemapModes, IM_ARRAYSIZE(kTonemapModes)))
            m_postSettings.tonemap.mode = static_cast<renderer::PostSettings::ToneMapMode>(tonemapMode);

        if (ImGui::BeginCombo("HDR", m_hdrFiles[m_selectedHdr].stem().string().c_str()))
        {
            for (int i = 0; i < m_hdrFiles.size(); ++i)
                if (ImGui::Selectable(m_hdrFiles[i].stem().string().c_str(), i == m_selectedHdr))
                {
                    m_selectedHdr = i;
                    m_pendingEnvironment = true;
                }
            ImGui::EndCombo();
        }

        ImGui::DragFloat("Shadow Normal Bias", &m_pipelineSettings.shadow.normalOffset, 0.05f, 0.0f, 20.0f);
        ImGui::DragFloat("Shadow Lambda", &m_pipelineSettings.shadow.lambda, 0.05f, 0.0f, 1.0f);
        ImGui::DragFloat("Shadow Distance", &m_pipelineSettings.shadow.distance, 10.5f, 0.0f, 500.0f);
        ImGui::DragFloat("Shadow Cater Margin", &m_pipelineSettings.shadow.casterMargin, 0.5f, 0.0f, 200.0f);
        ImGui::DragFloat("Shadow Bias Slope", &m_pipelineSettings.shadow.biasSlope, 0.5f, 0.0f, 200.0f);
        ImGui::End();
        m_device->endImGuiFrame();
        m_device->drawImGui(cmd);
        cmd->endRenderPass();

        cmd->present();
        m_device->endFrame(cmd);
    }
} // namespace nitro::studio

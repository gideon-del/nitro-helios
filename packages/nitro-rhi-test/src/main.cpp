#include <GLFW/glfw3.h>

#include <nitro-rhi/rhi-command-buffer.h>
#include <nitro-geometry/geometry.h>
#include <nitro-renderer/nitro-renderer.h>
#include <glm/gtc/matrix_transform.hpp>
#ifdef USE_METAL
#include <nitro-rhi-backends/metal/metal-device.h>
using DeviceType = nitro::rhi::metal::MetalDevice;
#else
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
using DeviceType = nitro::rhi::vulkan::VulkanDevice;
#endif
#include <iostream>
#include <string>
#include <imgui.h>
#include <random>
#include <ImGuizmo.h>
using namespace nitro::rhi;
using namespace nitro::geometry;
using namespace nitro::renderer;
using namespace nitro::assets;

constexpr float EPSILON = 1e-6f;

struct AppState
{
    OrbitalCamera *camera;
    bool mousePressed;
    double lastX, lastY;
};

void handleKeyboard(GLFWwindow *window, OrbitalCamera &camera)
{
    const float speed = 0.4f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        camera.moveForward(speed);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        camera.moveForward(-speed);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        camera.moveRight(-speed);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        camera.moveRight(speed);
    }
}

void addRandomSpheres(uint32_t count, float areaSize, Scene &scene, GPUMeshHandle mesh)
{
    std::mt19937 rng(50); // fixed seed
    std::uniform_real_distribution<float> pos(-areaSize, areaSize);

    for (int i = 0; i < count; i++)
    {

        auto transformation = MeshTransformation(glm::translate(glm::mat4(1.0f), glm::vec3(pos(rng), 10.0f, pos(rng))));
        scene.addMeshEntity(mesh, GPUMaterialHandle{}, transformation, "Sphere");
    }
}
void handleMouse(
    GLFWwindow *window,
    AppState &state,
    const ImGuiIO &io)
{
    if (io.WantCaptureMouse)
    {
        state.mousePressed = false;
        return;
    }

    bool pressed =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (pressed)
    {
        if (!state.mousePressed)
        {
            state.mousePressed = true;
            state.lastX = x;
            state.lastY = y;
        }
        else
        {
            state.camera->onMouseMove(
                x - state.lastX,
                y - state.lastY);

            state.lastX = x;
            state.lastY = y;
        }
    }
    else
    {
        state.mousePressed = false;
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(800, 600, "POST Processing", nullptr, nullptr);

    bool isMetal = false;
#ifdef USE_METAL
    isMetal = true;
#endif
    std::shared_ptr<DeviceType> device = std::make_shared<DeviceType>(window);
    std::shared_ptr<RHISwapchain> swapchain(
        device->createSwapchain(nullptr));
    auto entityStore = std::make_shared<EntityStore>();
    auto assetManager = std::make_shared<AssetManager>();
    auto gpuResourceCache = std::make_shared<GPUResourceCache>(device);
    auto materialManager = std::make_shared<MaterialManager>(device, assetManager, gpuResourceCache);
    auto meshManager = std::make_shared<MeshManager>(device, assetManager, entityStore);
    auto lightManager = std::make_shared<LightManager>(device, entityStore);
    Scene mainScene{device, meshManager, materialManager, assetManager, entityStore, lightManager};

    auto sphereMeshId = meshManager->addMesh(MeshGenerator::createUVSphere(5, 10, 100));

    // Mesh plane = MeshGenerator::createPlane(500, 500);
    // plane.calculateNormals();
    // auto planeMeshId = meshManager->addMesh(plane);

    // MeshInstance planeInstance;
    // planeInstance.meshId = planeMeshId;

    // mainScene.instanceIds.push_back(meshManager->addMeshInstances(planeInstance));
    // auto wallMeshId = meshManager->addMesh(MeshGenerator::createWallMesh(100, 100, 30));

    // MeshTransformation wallTransform;
    // wallTransform.translate(glm::vec3(0.0f, 10.0f, -100.0f));
    // auto wallPc = wallTransform.getTransform();

    // MeshInstance wallInstance;
    // wallInstance.meshId = wallMeshId;
    // wallInstance.modelTransform = wallPc.model;
    // wallInstance.normalTransform = wallPc.normalMatrix;

    // mainScene.instanceIds.push_back(meshManager->addMeshInstances(wallInstance));
    // pbrScene.objects.push_back(RenderObject(planeRenderer));
    addRandomSpheres(1, 10, mainScene, sphereMeshId);
    // addWallTestCluster(mainScene, sphereMeshId);
    Mesh pointLightSphere = MeshGenerator::createUVSphere(1, 10, 100);
    std::shared_ptr<MeshRenderer> pointLightRenderer = std::make_shared<MeshRenderer>(pointLightSphere, device);
    OrbitalCamera camera;
    // camera.radius = 10.0f;

    camera.setRadius(20.0f);
    OrbitalCamera light;
    light.setRadius(200.0f);
    light.setTheta(glm::radians(30.0f));
    light.setPhi(glm::radians(40.0f));
    AppState appState{
        .camera = &camera};

    glfwSetWindowUserPointer(window, &appState);

    glfwSetScrollCallback(window, [](GLFWwindow *w, double xoffset, double yoffset)
                          {

                                auto state = reinterpret_cast<AppState *>(glfwGetWindowUserPointer(w));

                                state->camera->onScroll(yoffset); });

    RHITimer *timer = device->createTimer();
    RendererSettings rendererSettings;

    // rendererSettings.light.pointLights = createRandomLights(10, 100);
    RenderContext renderContext;
    renderContext.camera = &camera;

    rendererSettings.light.lightCamera = light;
    rendererSettings.light.pointLightRenderer = pointLightRenderer;

    camera.setLens(
        60.0f,
        renderContext.CAMERA_NEAR,
        renderContext.CAMERA_FAR);
    // helmetScene.objects = Scene::loadGltfScene("./assets/buster_drone/scene.gltf", device, materialSystem);
    // ForwardRenderer forwardRenderer = ForwardRenderer(device, swapchain, std::string(SHADER_DIR), isMetal);
    // DeferredRenderer deferredRenderer = DeferredRenderer(device, swapchain, std::string(SHADER_DIR), isMetal, materialSystem);

    TiledDeferredRenderer tileDeferredRenderer = TiledDeferredRenderer(device, swapchain, std::string(SHADER_DIR), isMetal);

    renderContext.scene = &mainScene;
    renderContext.ssaoSamples.generate();

    camera.setFlipY(!isMetal);
    // IRenderer *currentRenderer = &deferredRenderer;

    int cachedWindowWidth, cachedWindowHeight;
    glfwGetFramebufferSize(window, &cachedWindowWidth, &cachedWindowHeight);
    uint32_t cachedViewportWidth = (uint32_t)rendererSettings.viewportSize.x;
    uint32_t cachedViewportHeight = (uint32_t)rendererSettings.viewportSize.y;
    glm::vec2 pendingViewport{0.0f};
    double pendingSince = 0.0;
    bool resizePending = false;
    glm::vec2 currentViewport{0.0f};
    // Build scene buffers

    mainScene.buildSceneInstanceId();

    materialManager->buildMegaMaterialBuffer();
    meshManager->buildMegaBuffers();
    rendererSettings.viewportSize = {(float)cachedViewportWidth, (float)cachedViewportHeight};
    while (!glfwWindowShouldClose(window))
    {

        glfwPollEvents();
        if (rendererSettings.pendingLoad)
        {
            if (mainScene.load(*rendererSettings.pendingLoad))
            {
                rendererSettings.currentScenePath = rendererSettings.pendingLoad.value();
            }
            rendererSettings.pendingLoad.reset();
        }
        if (rendererSettings.pendingImport)
        {
            device->waitIdle();
            mainScene.loadGltfScene(rendererSettings.pendingImport->string(), device);
            rendererSettings.pendingImport.reset();
        }
        auto currentTime = glfwGetTime();
        renderContext.deltaTime = std::max(currentTime - renderContext.lastFrameTime, 0.0001);
        renderContext.lastFrameTime = currentTime;

        ImGuiIO &io = ImGui::GetIO();

        glm::vec2 panel = rendererSettings.imGuiDockWindow;

        if (panel.x > 0.0f && panel.y > 0.0f && panel != currentViewport)
        {
            if (panel != pendingViewport)
            {
                pendingViewport = panel;
                pendingSince = currentTime;
                resizePending = true;
            }
            else if (resizePending && currentTime - pendingSince > 0.15)
            {
                device->waitIdle();
                rendererSettings.oldImGuiDockWindow = rendererSettings.imGuiDockWindow;
                rendererSettings.viewportSize = rendererSettings.imGuiDockWindow;

                camera.setViewPort(rendererSettings.imGuiDockWindow.x, rendererSettings.imGuiDockWindow.y);

                tileDeferredRenderer.resize(rendererSettings.imGuiDockWindow.x, rendererSettings.imGuiDockWindow.y);

                currentViewport = pendingViewport;
                resizePending = false;
            }
        }

        if (rendererSettings.viewportInputState.focused)
        {
            handleKeyboard(window, camera);
        }

        if (rendererSettings.viewportInputState.hovered && !ImGuizmo::IsUsing())
        {
            handleMouse(window, appState, io);
        }

        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        if ((windowWidth != cachedWindowWidth || windowHeight != cachedWindowHeight) && windowWidth > 0 && windowHeight > 0)
        {
            device->waitIdle();
            swapchain->resize(windowWidth, windowHeight);

            cachedWindowWidth = windowWidth;
            cachedWindowHeight = windowHeight;
        }

        RHICommandBuffer *cmd = device->beginFrame();
        meshManager->flusDirtyMeshInstances();
        materialManager->flush();
        lightManager->flush();
        renderContext.scene->flush();
        cmd->resetFrameStats();
        timer->beginFrame(cmd);

        timer->begin(cmd, "frame-time");

        tileDeferredRenderer.execute(cmd, renderContext, rendererSettings, timer);
        // switch (rendererSettings.renderer)
        // {
        // case RendererType::Forward:
        //     forwardRenderer.execute(
        //         cmd,
        //         renderContext,
        //         rendererSettings);
        //     break;
        // case RendererType::TiledDeferred:
        //     tileDeferredRenderer.execute(cmd, renderContext, rendererSettings);
        //     break;

        // case RendererType::Deferred:
        //     deferredRenderer.execute(
        //         cmd,
        //         renderContext,
        //         rendererSettings);

        //     break;
        // }

        auto frameStat = cmd->getFrameStats();
        timer->end(cmd, "frame-time");
        cmd->present();

        device->endFrame(cmd);
        timer->endFrame();
        rendererSettings.stats.fps = 1000.0f / timer->getResult("frame-time");
        rendererSettings.stats.frameTime = timer->getResult("frame-time");

        rendererSettings.stats.drawCalls = frameStat.drawCalls;
        rendererSettings.stats.triangles = frameStat.triangles;
        rendererSettings.stats.vertices = frameStat.vertices;
        rendererSettings.stats.perpassTimer = timer->getAllResults();
    }

    device->waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

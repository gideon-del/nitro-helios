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
using namespace nitro::rhi;
using namespace nitro::geometry;
using namespace nitro::renderer;

constexpr float EPSILON = 1e-6f;

struct AppState
{
    OrbitalCamera *camera;
    bool mousePressed;
    double lastX, lastY;
};

std::vector<PointLight> createRandomLights(
    uint32_t count,
    float areaSize)
{
    std::vector<PointLight> lights;

    std::mt19937 rng(42); // fixed seed
    std::uniform_real_distribution<float> pos(-areaSize, areaSize);
    std::uniform_real_distribution<float> color(0.2f, 1.0f);

    for (uint32_t i = 0; i < count; i++)
    {
        PointLight light;

        light.position = glm::vec4(
            pos(rng),
            20.0f,
            pos(rng),
            1.0f);

        light.color = glm::vec4(
            color(rng),
            color(rng),
            color(rng),
            1.0f);

        light.radius = 50.0f;
        light.intensity = 10.0f;

        lights.push_back(light);
    }

    return lights;
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

void addRandomSpheres(uint32_t count, float areaSize, Scene &scene, std::shared_ptr<MeshRenderer> renderer)
{
    std::mt19937 rng(50); // fixed seed
    std::uniform_real_distribution<float> pos(-areaSize, areaSize);

    for (int i = 0; i < count; i++)
    {
        scene.objects.push_back({renderer,
                                 MeshTransformation(glm::translate(glm::mat4(1.0f), glm::vec3(pos(rng), 10.0f, pos(rng))))});
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
    GLFWwindow *window = glfwCreateWindow(800, 600, "RHI Triangle", nullptr, nullptr);

    bool isMetal = false;
#ifdef USE_METAL
    isMetal = true;
#endif
    std::shared_ptr<DeviceType> device = std::make_shared<DeviceType>(window);
    std::shared_ptr<RHISwapchain> swapchain(
        device->createSwapchain(nullptr));

    Scene mainScene;
    Mesh sphere = MeshGenerator::createUVSphere(5, 10, 100);

    auto sphereRenderer = std::make_shared<MeshRenderer>(sphere, device);

    Mesh plane = MeshGenerator::createPlane(500, 500);
    plane.calculateNormals();
    auto planeRenderer = std::make_shared<MeshRenderer>(plane, device);
    mainScene.objects.push_back(RenderObject(planeRenderer));
    addRandomSpheres(400, 500, mainScene, sphereRenderer);
    Mesh pointLightSphere = MeshGenerator::createUVSphere(1, 10, 100);
    std::shared_ptr<MeshRenderer> pointLightRenderer = std::make_shared<MeshRenderer>(pointLightSphere, device);
    OrbitalCamera camera;
    // camera.radius = 10.0f;

    camera.radius = 3.0f;
    OrbitalCamera light;
    light.radius = 200.0f;
    light.theta = glm::radians(30.0f);
    light.phi = glm::radians(40.0f);
    AppState appState{
        .camera = &camera};

    glfwSetWindowUserPointer(window, &appState);

    glfwSetScrollCallback(window, [](GLFWwindow *w, double xoffset, double yoffset)
                          {

                              auto state = reinterpret_cast<AppState *>(glfwGetWindowUserPointer(w));

                              state->camera->onScroll(yoffset); });

    RHITimer *timer = device->createTimer();
    RendererSettings rendererSettings;
    rendererSettings.light.pointLights = createRandomLights(1000, 500);
    RenderContext renderContext;
    renderContext.camera = &camera;
    renderContext.scene = &mainScene;

    rendererSettings.light.lightCamera = light;
    rendererSettings.light.pointLightRenderer = pointLightRenderer;
    ForwardRenderer forwardRenderer = ForwardRenderer(device, swapchain, std::string(SHADER_DIR), isMetal);
    DeferredRenderer deferredRenderer = DeferredRenderer(device, swapchain, std::string(SHADER_DIR), isMetal);
    TiledDeferredRenderer tileDeferredRenderer = TiledDeferredRenderer(device, swapchain, std::string(SHADER_DIR), isMetal);
    IRenderer *currentRenderer = &deferredRenderer;
    int cachedWidth, cachedHeight;
    glfwGetFramebufferSize(window, &cachedWidth, &cachedHeight);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGuiIO &io = ImGui::GetIO();

        if (!io.WantCaptureKeyboard)
        {
            handleKeyboard(window, camera);
        }

        if (!io.WantCaptureMouse)
        {
            handleMouse(window, appState, io);
        }

        GlobalTransformation globalUbo{};
        int width, height;

        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        if ((cachedWidth != width || cachedHeight != height) && width > 0 && height > 0)
        {
            device->waitIdle();
            swapchain->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            forwardRenderer.resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            deferredRenderer.resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            tileDeferredRenderer.resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            cachedWidth = width;
            cachedHeight = height;
        };
        RHICommandBuffer *cmd = device->beginFrame();
        cmd->resetFrameStats();
        timer->beginFrame(cmd);

        timer->begin(cmd, "frame-time");

        switch (rendererSettings.renderer)
        {
        case RendererType::Forward:
            forwardRenderer.execute(
                cmd,
                renderContext,
                rendererSettings);
            break;
        case RendererType::TiledDeferred:
            tileDeferredRenderer.execute(cmd, renderContext, rendererSettings);
            break;

        case RendererType::Deferred:
            deferredRenderer.execute(
                cmd,
                renderContext,
                rendererSettings);

            break;
        }
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
    }

    device->waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
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

    Mesh plane = MeshGenerator::createPlane(50, 50);
    plane.calculateNormals();
    auto planeRenderer = std::make_shared<MeshRenderer>(plane, device);
    mainScene.objects.push_back(
        RenderObject(
            planeRenderer,
            MeshTransformation()));
    mainScene.objects.push_back(
        RenderObject(
            sphereRenderer,
            MeshTransformation(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f)))));
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

    glfwSetCursorPosCallback(window, [](GLFWwindow *w, double x, double y)
                             {
     auto state = reinterpret_cast<AppState *>(glfwGetWindowUserPointer(w));
     if(state->mousePressed){
        state->camera->onMouseMove(x - state->lastX, y - state->lastY);
        state->lastX =x;
        state->lastY = y;
     } });

    glfwSetScrollCallback(window, [](GLFWwindow *w, double xoffset, double yoffset)
                          {

                              auto state = reinterpret_cast<AppState *>(glfwGetWindowUserPointer(w));

                              state->camera->onScroll(yoffset); });

    glm::vec3 sphereCenter(0, 10, 0);
    glm::vec4 sphereLS =
        light.getView() * glm::vec4(sphereCenter, 1.0f);
    RHITimer *timer = device->createTimer();
    RendererSettings rendererSettings;
    RenderContext renderContext;
    renderContext.camera = &camera;
    renderContext.scene = &mainScene;

    rendererSettings.light.lightCamera = light;
    ForwardRenderer forwardRenderer = ForwardRenderer(device, swapchain, std::string(SHADER_DIR), isMetal);
    DeferredRenderer deferredRenderer = DeferredRenderer(device, swapchain, std::string(SHADER_DIR), isMetal);
    IRenderer *currentRenderer = &deferredRenderer;
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
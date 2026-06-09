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

    // glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
    //                    {
    //                        auto state = reinterpret_cast<AppState *>(glfwGetWindowUserPointer(window));

    //                        if (key == GLFW_KEY_W || key == GLFW_KEY_UP)
    //                        {
    //                            state->camera->moveForward(0.4f);
    //                        }

    //                        if (key == GLFW_KEY_S || key == GLFW_KEY_DOWN)
    //                        {
    //                            state->camera->moveForward(-0.4f);
    //                        }
    //                        if (key == GLFW_KEY_A || key == GLFW_KEY_RIGHT)
    //                        {
    //                            state->camera->moveRight(0.4f);
    //                        }

    //                        if (key == GLFW_KEY_D || key == GLFW_KEY_LEFT)
    //                        {
    //                            state->camera->moveRight(-0.4f);
    //                        } });
    // glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int button, int action, int mods)
    //                            {
    //                                auto state = reinterpret_cast<AppState *>(glfwGetWindowUserPointer(w));

    //                                if (button == GLFW_MOUSE_BUTTON_LEFT)
    //                                {
    //                                    state->mousePressed = (action == GLFW_PRESS);
    //                                    glfwGetCursorPos(w, &state->lastX, &state->lastY);
    //                                } });
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
    IRenderer *currentRenderer = &forwardRenderer;
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
        timer->beginFrame(cmd);
        currentRenderer->execute(cmd, renderContext, rendererSettings);

        // uint32_t frameIdx = device.getCurrentFrameIndex();
        // FrameResource &frameResource = frameResources[frameIdx];

        // LightView lightView;
        // for (int i = 0; i < cascades.size(); i++)
        // {
        //     lightView.lightSpaceMatrix[i] = ShadowPass::s_calculateLightOrthoProj(
        //         CAMERA_NEAR,
        //         CAMERA_FAR,
        //         4,
        //         i,
        //         glm::radians(60.0f),
        //         aspect,
        //         camera.getView(),
        //         camera.getEye(),
        //         light.getView(), shadowSettings.lambda);
        //     frameData.cascadeSplit[i] = ShadowPass::s_getPracticalSplit(CAMERA_NEAR, CAMERA_FAR, 4, i + 1, shadowSettings.lambda);
        // }

        // frameResource.getBuffer(FrameResourceId::LightUniformBuffer)->upload(&lightView, sizeof(LightView));
        // for (int i = 0; i < cascades.size(); i++)
        // {
        //     cascades[i].execute(cmd, shadowPipeline, frameResource.getDescriptorSet(FrameResourceId::ShadowDescriptorSet), mainScene);
        // }
        // RHIRenderPassDesc rpDesc{};
        // rpDesc.clearColor[0] = 0.3f;
        // rpDesc.clearColor[1] = 0.3f;
        // rpDesc.clearColor[2] = 0.3f;
        // rpDesc.clearColor[3] = 1.0f;
        // rpDesc.clearDepth = 1.0f;
        // rpDesc.hasDepth = true;
        // globalUbo.proj = glm::perspective(glm::radians(60.0f), aspect, CAMERA_NEAR, CAMERA_FAR);
        // globalUbo.view = camera.getView();

        // if (!isMetal)
        // {
        //     globalUbo.proj[1][1] *= -1.0f;
        // }

        // frameData.view = camera.getView();
        // frameData.cameraPos = glm::vec4(camera.getEye(), 1.0f);

        // GeometryCameraBuffer geometryCameraBuffer;
        // geometryCameraBuffer.proj = globalUbo.proj;
        // geometryCameraBuffer.view = camera.getView();
        // geometryPass.execute(cmd, geometryCameraBuffer, mainScene);
        // frameData.proj = globalUbo.proj;
        // frameData.lightPos = glm::vec4(light.getEye(), 1.0f);
        // for (int i = 0; i < cascades.size(); i++)
        // {
        //     frameData.lightViewProj[i] = lightView.lightSpaceMatrix[i];
        // }
        // cmd->beginRenderPass(rpDesc);
        // cmd->bindPipeline(pipeline);

        // frameData.ambient = rendererSettings.ambient;
        // frameData.Ka = rendererSettings.Ka;
        // frameData.Kd = rendererSettings.Kd;
        // frameData.Ks = rendererSettings.Ks;
        // frameData.lightColor =
        //     glm::vec4(rendererSettings.lightColor, 1.0f);

        // frameData.shadowBias = shadowSettings.bias;
        // frameData.shadowNormalBias = shadowSettings.normalBias;
        // frameData.showCascadeColors = shadowSettings.showCascadeColors ? 1.0f : 0.0f;
        // frameResource.getBuffer(FrameResourceId::FrameDataUniformBuffer)->upload(&frameData, sizeof(FrameData));

        // cmd->bindDescriptorSet(frameResource.getDescriptorSet(FrameResourceId::MainDescriptorSet), 0);
        // cmd->bindDescriptorSet(frameResource.getDescriptorSet(FrameResourceId::TextureDescriptor), 1);
        // RHIViewport mainViewPort;
        // mainViewPort.width = swapchain->getWidth();
        // mainViewPort.height = swapchain->getHeight();
        // cmd->setViewPort(mainViewPort);
        // RHIScissor mainScissor;
        // mainScissor.width = swapchain->getWidth();
        // mainScissor.height = swapchain->getHeight();
        // cmd->setScissor(mainScissor);
        // mainScene.draw(cmd);

        // device.beginImGuiFrame();

        // rendererPanel.draw(rendererSettings, light);
        // shadowPanel.draw(shadowSettings);
        // device.endImGuiFrame();
        // device.drawImGui(cmd);
        // cmd->endRenderPass();

        cmd->present();

        device->endFrame(cmd);
        timer->endFrame();
        // std::cout << "shadow: " << timer->getResult("shadow_pass") << " ms\n";
    }

    device->waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
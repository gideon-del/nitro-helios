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

struct LightView
{
    glm::mat4 lightSpaceMatrix[4];
};

struct FrameData
{
    glm::mat4 view;
    glm::mat4 proj;

    glm::vec4 cameraPos;

    glm::vec4 lightPos;
    glm::vec4 lightColor = glm::vec4(1.0f);

    glm::mat4 lightViewProj[4];
    glm::vec4 cascadeSplit;

    float ambient = 0.3f;
    float Ka = 1.0f;
    float Kd = 0.8f;
    float Ks = 0.9f;
    float shininess = 32.0f;

    float shadowBias;
    float shadowNormalBias;
    float showCascadeColors;
    float padding;
};

struct RendererSettings
{
    float ambient = 0.3f;
    float Ka = 1.0f;
    float Kd = 0.8f;
    float Ks = 0.9f;
    float shininess = 32.0f;

    glm::vec3 lightColor = glm::vec3(1.0f);
};

struct RendererPanel
{
    void draw(
        RendererSettings &settings,
        OrbitalCamera &light)
    {
        ImGui::Begin("Renderer");

        if (ImGui::CollapsingHeader("Light Camera"))
        {
            ImGui::SliderFloat(
                "Phi",
                &light.phi,
                0.0f,
                2 * M_PI);

            ImGui::SliderFloat(
                "Theta",
                &light.theta,
                0.0f,
                M_PI);

            ImGui::SliderFloat(
                "Radius",
                &light.radius,
                0.0f,
                100.0f);
        }

        if (ImGui::CollapsingHeader("Lighting"))
        {
            ImGui::SliderFloat(
                "Ambient",
                &settings.ambient,
                0,
                1);

            ImGui::SliderFloat(
                "Ka",
                &settings.Ka,
                0,
                1);

            ImGui::SliderFloat(
                "Kd",
                &settings.Kd,
                0,
                1);

            ImGui::SliderFloat(
                "Ks",
                &settings.Ks,
                0,
                1);

            ImGui::ColorEdit3(
                "Light Color",
                &settings.lightColor.x);
        }

        ImGui::End();
    }
};

struct ShadowSettings
{
    float bias = 0.005f;
    float normalBias = 0.05f;

    float lambda = 0.5f;

    bool showCascadeColors = false;
};

struct ShadowPanel
{
    void draw(ShadowSettings &settings)
    {
        ImGui::Begin("Shadows");

        ImGui::SliderFloat(
            "Bias",
            &settings.bias,
            0.0f,
            0.02f);

        ImGui::SliderFloat(
            "Normal Bias",
            &settings.normalBias,
            0.0f,
            0.2f);

        ImGui::SliderFloat(
            "Lambda",
            &settings.lambda,
            0.0f,
            1.0f);

        ImGui::Checkbox(
            "Show Cascade Colors",
            &settings.showCascadeColors);

        ImGui::End();
    }
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
    DeviceType device(window);
    RHISwapchain *swapchain = device.createSwapchain(nullptr);

    std::string mainShaderPath = std::string(SHADER_DIR) + "/main/main";
    std::string shadowShaderPath = std::string(SHADER_DIR) + "/shadow/shadow";

    std::vector<RHIDescriptorBinding> binding = {
        {.type = RHIDescriptorBinding::Type::UniformBuffer,
         .stage = RHIDescriptorBinding::ShaderStage::Both,
         .binding = 2},
    };

    std::vector<RHIDescriptorBinding> shadowBinding = {
        {.type = RHIDescriptorBinding::Type::UniformBuffer,
         .stage = RHIDescriptorBinding::ShaderStage::Vertex,
         .binding = 2},
    };

    std::vector<RHIDescriptorBinding> shadowTextureBinding = {
        {.type = RHIDescriptorBinding::Type::Sampler,
         .stage = RHIDescriptorBinding::ShaderStage::Fragment,
         .binding = 0},
        {.type = RHIDescriptorBinding::Type::Sampler,
         .stage = RHIDescriptorBinding::ShaderStage::Fragment,
         .binding = 1},
        {.type = RHIDescriptorBinding::Type::Sampler,
         .stage = RHIDescriptorBinding::ShaderStage::Fragment,
         .binding = 2},
        {.type = RHIDescriptorBinding::Type::Sampler,
         .stage = RHIDescriptorBinding::ShaderStage::Fragment,
         .binding = 3},

    };
    RHIDescriptorLayout *descriptorLayout = device.createDescriptorLayout(binding);
    RHIDescriptorLayout *shadowDescriptorLayout = device.createDescriptorLayout(shadowBinding);
    RHIDescriptorLayout *textureDescriptorLayout = device.createDescriptorLayout(shadowTextureBinding);
    PipelineDesc shadowPipelineDesc{};
    PipelineDesc pipelineDesc{};
#ifdef USE_METAL
    pipelineDesc.shaders.push_back({"vs", mainShaderPath + ".metallib", ShaderStage::Vertex});
    pipelineDesc.shaders.push_back({"fs", mainShaderPath + ".metallib", ShaderStage::Fragment});
    shadowPipelineDesc.shaders.push_back({"vs", shadowShaderPath + ".metallib", ShaderStage::Vertex});
#else
    pipelineDesc.shaders.push_back({"main", mainShaderPath + ".vert.spv", ShaderStage::Vertex});
    pipelineDesc.shaders.push_back({"main", mainShaderPath + ".frag.spv", ShaderStage::Fragment});
    shadowPipelineDesc.shaders.push_back({"main", shadowShaderPath + ".vert.spv", ShaderStage::Vertex});
#endif
    pipelineDesc.vertexLayout = Vertex::getVertexLayout();
    pipelineDesc.depthTest = true;
    pipelineDesc.layouts = {descriptorLayout, textureDescriptorLayout};
    pipelineDesc.pushConstantSize = sizeof(PushConstant);
    RHIPipeline *pipeline = device.createPipeline(pipelineDesc);

    Scene mainScene;
    Mesh sphere = MeshGenerator::createUVSphere(5, 10, 100);

    auto sphereRenderer = std::make_shared<MeshRenderer>(sphere, &device);

    Mesh plane = MeshGenerator::createPlane(50, 50);
    plane.calculateNormals();
    auto planeRenderer = std::make_shared<MeshRenderer>(plane, &device);
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

    shadowPipelineDesc.vertexLayout = Vertex::getVertexLayout();
    shadowPipelineDesc.depthTest = true;
    shadowPipelineDesc.hasColorAttachment = false;
    shadowPipelineDesc.layouts = {shadowDescriptorLayout};
    shadowPipelineDesc.pushConstantSize = sizeof(ShadowPushConstant);
    RHIPipeline *shadowPipeline = device.createPipeline(shadowPipelineDesc);

    float CAMERA_NEAR = 0.1f;
    float CAMERA_FAR = 100.0f;

    BufferDesc lightUboDesc;
    lightUboDesc.size = sizeof(LightView);
    lightUboDesc.storage = BufferDesc::StorageMode::Shared;
    lightUboDesc.usage = BufferDesc::Usage::Uniform;

    BufferDesc frameDataUBODesc;
    frameDataUBODesc.size = sizeof(FrameData);
    frameDataUBODesc.storage = BufferDesc::StorageMode::Shared;
    frameDataUBODesc.usage = BufferDesc::Usage::Uniform;

    std::vector<FrameResource> frameResources;
    std::vector<ShadowPass> cascades;

    for (int i = 0; i < 4; i++)
    {
        cascades.push_back(ShadowPass(&device, i));
    }
    for (int i = 0; i < 2; i++)
    {
        FrameResource frameResource(&device);
        frameResource.setBuffer(FrameResourceId::LightUniformBuffer, device.createBuffer(lightUboDesc));
        frameResource.setBuffer(FrameResourceId::FrameDataUniformBuffer, device.createBuffer(frameDataUBODesc));
        RHIDescriptorSet *shadowDescriptorSet = device.createDescriptorSet(shadowDescriptorLayout);

        shadowDescriptorSet->writeBuffer(frameResource.getBuffer(FrameResourceId::LightUniformBuffer), 2);
        shadowDescriptorSet->commit();

        frameResource.setDescriptorSet(FrameResourceId::ShadowDescriptorSet, shadowDescriptorSet);

        RHIDescriptorSet *mainDescriptorSet = device.createDescriptorSet(descriptorLayout);
        mainDescriptorSet->writeBuffer(frameResource.getBuffer(FrameResourceId::FrameDataUniformBuffer), 2);
        mainDescriptorSet->commit();

        RHIDescriptorSet *textureDescriptorSet = device.createDescriptorSet(textureDescriptorLayout);
        for (int i = 0; i < cascades.size(); i++)
        {
            textureDescriptorSet->writeTexture(cascades[i].shadowTexture, i);
        }
        textureDescriptorSet->commit();
        frameResource.setDescriptorSet(FrameResourceId::MainDescriptorSet, mainDescriptorSet);
        frameResource.setDescriptorSet(FrameResourceId::TextureDescriptor, textureDescriptorSet);

        frameResources.push_back(std::move(frameResource));
    }
    double lastTime = glfwGetTime();
    double currentTime = lastTime;

    auto lightEye = light.getEye();

    glm::vec3 sphereCenter(0, 10, 0);
    glm::vec4 sphereLS =
        light.getView() * glm::vec4(sphereCenter, 1.0f);
    RHITimer *timer = device.createTimer();
    FrameData frameData;
    RendererSettings rendererSettings;
    RendererPanel rendererPanel;
    ShadowSettings shadowSettings;
    ShadowPanel shadowPanel;
    GeometryPass geometryPass(&device, swapchain->getWidth(), swapchain->getHeight(), std::string(SHADER_DIR), isMetal);
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
        currentTime = glfwGetTime();
        // light.phi = float(glfwGetTime() * glm::radians(20.0f));

        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        RHICommandBuffer *cmd = device.beginFrame();
        timer->beginFrame(cmd);

        uint32_t frameIdx = device.getCurrentFrameIndex();
        FrameResource &frameResource = frameResources[frameIdx];

        LightView lightView;
        for (int i = 0; i < cascades.size(); i++)
        {
            lightView.lightSpaceMatrix[i] = ShadowPass::s_calculateLightOrthoProj(
                CAMERA_NEAR,
                CAMERA_FAR,
                4,
                i,
                glm::radians(60.0f),
                aspect,
                camera.getView(),
                camera.getEye(),
                light.getView(), shadowSettings.lambda);
            frameData.cascadeSplit[i] = ShadowPass::s_getPracticalSplit(CAMERA_NEAR, CAMERA_FAR, 4, i + 1, shadowSettings.lambda);
        }

        frameResource.getBuffer(FrameResourceId::LightUniformBuffer)->upload(&lightView, sizeof(LightView));
        for (int i = 0; i < cascades.size(); i++)
        {
            cascades[i].execute(cmd, shadowPipeline, frameResource.getDescriptorSet(FrameResourceId::ShadowDescriptorSet), mainScene);
        }
        RHIRenderPassDesc rpDesc{};
        rpDesc.clearColor[0] = 0.3f;
        rpDesc.clearColor[1] = 0.3f;
        rpDesc.clearColor[2] = 0.3f;
        rpDesc.clearColor[3] = 1.0f;
        rpDesc.clearDepth = 1.0f;
        rpDesc.hasDepth = true;
        globalUbo.proj = glm::perspective(glm::radians(60.0f), aspect, CAMERA_NEAR, CAMERA_FAR);
        globalUbo.view = camera.getView();

        if (!isMetal)
        {
            globalUbo.proj[1][1] *= -1.0f;
        }

        frameData.view = camera.getView();
        frameData.cameraPos = glm::vec4(camera.getEye(), 1.0f);

        GeometryCameraBuffer geometryCameraBuffer;
        geometryCameraBuffer.proj = globalUbo.proj;
        geometryCameraBuffer.view = camera.getView();
        geometryPass.execute(cmd, geometryCameraBuffer, mainScene);
        frameData.proj = globalUbo.proj;
        frameData.lightPos = glm::vec4(light.getEye(), 1.0f);
        for (int i = 0; i < cascades.size(); i++)
        {
            frameData.lightViewProj[i] = lightView.lightSpaceMatrix[i];
        }
        cmd->beginRenderPass(rpDesc);
        cmd->bindPipeline(pipeline);

        frameData.ambient = rendererSettings.ambient;
        frameData.Ka = rendererSettings.Ka;
        frameData.Kd = rendererSettings.Kd;
        frameData.Ks = rendererSettings.Ks;
        frameData.lightColor =
            glm::vec4(rendererSettings.lightColor, 1.0f);

        frameData.shadowBias = shadowSettings.bias;
        frameData.shadowNormalBias = shadowSettings.normalBias;
        frameData.showCascadeColors = shadowSettings.showCascadeColors ? 1.0f : 0.0f;
        frameResource.getBuffer(FrameResourceId::FrameDataUniformBuffer)->upload(&frameData, sizeof(FrameData));

        cmd->bindDescriptorSet(frameResource.getDescriptorSet(FrameResourceId::MainDescriptorSet), 0);
        cmd->bindDescriptorSet(frameResource.getDescriptorSet(FrameResourceId::TextureDescriptor), 1);
        RHIViewport mainViewPort;
        mainViewPort.width = swapchain->getWidth();
        mainViewPort.height = swapchain->getHeight();
        cmd->setViewPort(mainViewPort);
        RHIScissor mainScissor;
        mainScissor.width = swapchain->getWidth();
        mainScissor.height = swapchain->getHeight();
        cmd->setScissor(mainScissor);
        mainScene.draw(cmd);

        device.beginImGuiFrame();

        rendererPanel.draw(rendererSettings, light);
        shadowPanel.draw(shadowSettings);
        device.endImGuiFrame();
        device.drawImGui(cmd);
        cmd->endRenderPass();

        cmd->present();

        device.endFrame(cmd);
        timer->endFrame();
        // std::cout << "shadow: " << timer->getResult("shadow_pass") << " ms\n";
    }

    device.waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
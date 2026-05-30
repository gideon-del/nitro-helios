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
    glm::mat4 lightSpaceMatrix;
};

struct FrameData
{
    glm::mat4 view;
    glm::mat4 proj;

    glm::vec4 cameraPos;

    glm::vec4 lightPos;
    glm::vec4 lightColor = glm::vec4(1.0f);

    glm::mat4 lightViewProj;

    float ambient = 0.3f;
    float Ka = 1.0f;
    float Kd = 0.8f;
    float Ks = 0.9f;
    float shininess = 120.0f;
};
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

    std::string shaderPath = std::string(SHADER_DIR) + "/cube/cube";
    std::string shadowShaderPath = std::string(SHADER_DIR) + "/shadow/shadow";

    std::vector<RHIDescriptorBinding> binding = {
        // {.type = RHIDescriptorBinding::Type::UniformBuffer,
        //  .stage = RHIDescriptorBinding::ShaderStage::Vertex,
        //  .binding = 2},
        // {.type = RHIDescriptorBinding::Type::UniformBuffer,
        //  .stage = RHIDescriptorBinding::ShaderStage::Vertex,
        //  .binding = 3},
        {.type = RHIDescriptorBinding::Type::Sampler,
         .stage = RHIDescriptorBinding::ShaderStage::Fragment,
         .binding = 0},
        {.type = RHIDescriptorBinding::Type::UniformBuffer,
         .stage = RHIDescriptorBinding::ShaderStage::Both,
         .binding = 2},
        // {.type = RHIDescriptorBinding::Type::UniformBuffer,
        //  .stage = RHIDescriptorBinding::ShaderStage::Fragment,
        //  .binding = 5},

    };

    std::vector<RHIDescriptorBinding> shadowBinding = {
        {.type = RHIDescriptorBinding::Type::UniformBuffer,
         .stage = RHIDescriptorBinding::ShaderStage::Vertex,
         .binding = 2},
    };

    RHIDescriptorLayout *descriptorLayout = device.createDescriptorLayout(binding);
    RHIDescriptorLayout *shadowDescriptorLayout = device.createDescriptorLayout(shadowBinding);
    PipelineDesc shadowPipelineDesc{};
    PipelineDesc pipelineDesc{};
#ifdef USE_METAL
    pipelineDesc.shaders.push_back({"vs", shaderPath + ".metallib", ShaderStage::Vertex});
    pipelineDesc.shaders.push_back({"fs", shaderPath + ".metallib", ShaderStage::Fragment});
    shadowPipelineDesc.shaders.push_back({"main", shadowShaderPath + ".metallib", ShaderStage::Vertex});
#else
    pipelineDesc.shaders.push_back({"main", shaderPath + ".vert.spv", ShaderStage::Vertex});
    pipelineDesc.shaders.push_back({"main", shaderPath + ".frag.spv", ShaderStage::Fragment});
    shadowPipelineDesc.shaders.push_back({"main", shadowShaderPath + ".vert.spv", ShaderStage::Vertex});
#endif
    pipelineDesc.vertexLayout = Vertex::getVertexLayout();
    pipelineDesc.depthTest = true;
    pipelineDesc.layout = descriptorLayout;
    RHIPipeline *pipeline = device.createPipeline(pipelineDesc);
    pipelineDesc.topology = PipelineTopology::LineList;
    RHIPipeline *normalPipeline = device.createPipeline(pipelineDesc);

    Mesh sphere = MeshGenerator::createUVSphere(5, 10, 100);
    sphere.calculateNormals();
    MeshRenderer sphereRenderer(sphere, &device);
    sphereRenderer.transformation.translate(
        glm::vec3(0.0f, -5.0f, 0.0f));
    Mesh plane = MeshGenerator::createPlane(50, 50);
    plane.calculateNormals();
    MeshRenderer planeRenderer(plane, &device);
    planeRenderer.transformation.translate(glm::vec3(0.0f, -10.0f, 0.0f));

    OrbitalCamera camera;
    OrbitalCamera light;
    light.radius = 20.0f;
    light.theta = glm::radians(50.0f);
    light.phi = glm::radians(270.0f);

    AppState appState{
        .camera = &camera};

    glfwSetWindowUserPointer(window, &appState);

    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
                       {
                           auto state = reinterpret_cast<AppState *>(glfwGetWindowUserPointer(window));

                           if (key == GLFW_KEY_W || key == GLFW_KEY_UP)
                           {
                               state->camera->moveForward(0.4f);
                           }

                           if (key == GLFW_KEY_S || key == GLFW_KEY_DOWN)
                           {
                               state->camera->moveForward(-0.4f);
                           }
                           if (key == GLFW_KEY_A || key == GLFW_KEY_RIGHT)
                           {
                               state->camera->moveRight(0.4f);
                           }

                           if (key == GLFW_KEY_D || key == GLFW_KEY_LEFT)
                           {
                               state->camera->moveRight(-0.4f);
                           } });
    glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int button, int action, int mods)
                               {
                                   auto state = reinterpret_cast<AppState *>(glfwGetWindowUserPointer(w));

                                   if (button == GLFW_MOUSE_BUTTON_LEFT)
                                   {
                                       state->mousePressed = (action == GLFW_PRESS);
                                       glfwGetCursorPos(w, &state->lastX, &state->lastY);
                                   } });
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

    TextureDesc shadowDesc{};
    shadowDesc.size.width = 2048;
    shadowDesc.size.height = 2048;
    shadowDesc.format = TextureDesc::ImageFormat::Depth32Float;
    shadowDesc.usage = TextureDesc::Usage::DepthStencil | TextureDesc::Usage::ShaderRead;

    RHITexture *shadowDepthTexture = device.createTexture(shadowDesc);
    shadowPipelineDesc.vertexLayout = Vertex::getVertexLayout();
    shadowPipelineDesc.depthTest = true;
    shadowPipelineDesc.hasColorAttachment = false;
    shadowPipelineDesc.layout = shadowDescriptorLayout;

    RHIPipeline *shadowPipeline = device.createPipeline(shadowPipelineDesc);
    RenderPassDesc shadowRenderPassDesc;
    RenderPassDesc::Attachment shadowDepthAttachment;
    shadowDepthAttachment.texture = shadowDepthTexture;
    shadowDepthAttachment.store = RenderPassDesc::StoreOp::Store;
    shadowRenderPassDesc.depthAttachment = &shadowDepthAttachment;

    RHIRenderPass *shadowRenderPass = device.createRenderPass(shadowRenderPassDesc);

    // BufferDesc globalUBODesc;
    // globalUBODesc.size = sizeof(GlobalTransformation);
    // globalUBODesc.storage = BufferDesc::StorageMode::Shared;
    // globalUBODesc.usage = BufferDesc::Usage::Uniform;

    BufferDesc lightUboDesc;
    lightUboDesc.size = sizeof(LightView);
    lightUboDesc.storage = BufferDesc::StorageMode::Shared;
    lightUboDesc.usage = BufferDesc::Usage::Uniform;

    // BufferDesc directionalLightUboDesc;
    // lightUboDesc.size = sizeof(BlinnPhongLight);
    // lightUboDesc.storage = BufferDesc::StorageMode::Shared;
    // lightUboDesc.usage = BufferDesc::Usage::Uniform;

    // BufferDesc cameraViewUboDesc;
    // lightUboDesc.size = sizeof(CameraView);
    // lightUboDesc.storage = BufferDesc::StorageMode::Shared;
    // lightUboDesc.usage = BufferDesc::Usage::Uniform;

    BufferDesc frameDataUBODesc;
    frameDataUBODesc.size = sizeof(FrameData);
    frameDataUBODesc.storage = BufferDesc::StorageMode::Shared;
    frameDataUBODesc.usage = BufferDesc::Usage::Uniform;

    std::vector<FrameResource> frameResources;

    for (int i = 0; i < 2; i++)
    {
        FrameResource frameResource(&device);
        // frameResource.setBuffer(FrameResourceId::GlobalUniformBuffer, device.createBuffer(globalUBODesc));
        frameResource.setBuffer(FrameResourceId::LightUniformBuffer, device.createBuffer(lightUboDesc));
        frameResource.setBuffer(FrameResourceId::FrameDataUniformBuffer, device.createBuffer(frameDataUBODesc));
        // frameResource.setBuffer(FrameResourceId::DirectionalLightUniformBuffer, device.createBuffer(directionalLightUboDesc));
        // frameResource.setBuffer(FrameResourceId::CameraViewUniformBuffer, device.createBuffer(cameraViewUboDesc));

        RHIDescriptorSet *shadowDescriptorSet = device.createDescriptorSet(shadowDescriptorLayout);

        shadowDescriptorSet->writeBuffer(frameResource.getBuffer(FrameResourceId::LightUniformBuffer), 2);
        shadowDescriptorSet->commit();

        frameResource.setDescriptorSet(FrameResourceId::ShadowDescriptorSet, shadowDescriptorSet);

        RHIDescriptorSet *mainDescriptorSet = device.createDescriptorSet(descriptorLayout);
        // mainDescriptorSet->writeBuffer(frameResource.getBuffer(FrameResourceId::GlobalUniformBuffer), 2);
        // mainDescriptorSet->writeBuffer(frameResource.getBuffer(FrameResourceId::LightUniformBuffer), 3);
        // mainDescriptorSet->writeBuffer(frameResource.getBuffer(FrameResourceId::DirectionalLightUniformBuffer), 4);
        mainDescriptorSet->writeBuffer(frameResource.getBuffer(FrameResourceId::FrameDataUniformBuffer), 2);
        mainDescriptorSet->writeTexture(shadowDepthTexture, 0);
        mainDescriptorSet->commit();

        frameResource.setDescriptorSet(FrameResourceId::MainDescriptorSet, mainDescriptorSet);

        frameResources.push_back(std::move(frameResource));
    }
    double lastTime = glfwGetTime();
    double currentTime = lastTime;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        GlobalTransformation globalUbo{};
        int width, height;
        currentTime = glfwGetTime();
        light.phi = float(glfwGetTime() * glm::radians(20.0f));

        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        RHICommandBuffer *cmd = device.beginFrame();

        LightView lightView;

        lightView.lightSpaceMatrix = glm::orthoRH_ZO(-light.radius,
                                                     light.radius,
                                                     -light.radius,
                                                     light.radius,
                                                     0.1f,
                                                     50.0f) *
                                     glm::lookAt(light.getEye(), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // if (!isMetal)
        // {
        //     lightView.lightSpaceMatrix[1][1] *= -1;
        // }

        BlinnPhongLight bpLight;
        bpLight.lightPosition = glm::vec4(light.getEye(), 1.0f);
        CameraView cameraView;
        cameraView.view = glm::vec4(camera.getEye(), 1.0f);

        FrameData frameData;
        frameData.view = camera.getView();

        cmd->beginRenderPass(shadowRenderPass);
        cmd->bindPipeline(shadowPipeline);
        uint32_t frameIdx = device.getCurrentFrameIndex();
        FrameResource &frameResource = frameResources[frameIdx];

        frameResource.getBuffer(FrameResourceId::LightUniformBuffer)->upload(&lightView, sizeof(LightView));

        cmd->bindDescriptorSet(frameResource.getDescriptorSet(FrameResourceId::ShadowDescriptorSet));
        planeRenderer.draw(cmd);
        sphereRenderer.draw(cmd);
        cmd->endRenderPass();

        RHIRenderPassDesc rpDesc{};
        rpDesc.clearColor[0] = 0.2f;
        rpDesc.clearColor[1] = 0.2f;
        rpDesc.clearColor[2] = 0.2f;
        rpDesc.clearColor[3] = 1.0f;
        rpDesc.clearDepth = 1.0f;
        rpDesc.hasDepth = true;
        globalUbo.proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
        globalUbo.view = camera.getView();

        if (!isMetal)
        {
            globalUbo.proj[1][1] *= -1;
        };
        frameData.proj = globalUbo.proj;
        frameData.lightPos = glm::vec4(light.getEye(), 1.0f);
        frameData.lightViewProj = lightView.lightSpaceMatrix;
        cmd->beginRenderPass(rpDesc);
        cmd->bindPipeline(pipeline);

        // frameResource.getBuffer(FrameResourceId::GlobalUniformBuffer)->upload(&globalUbo, sizeof(GlobalTransformation));
        // frameResource.getBuffer(FrameResourceId::DirectionalLightUniformBuffer)->upload(&bpLight, sizeof(BlinnPhongLight));
        frameResource.getBuffer(FrameResourceId::FrameDataUniformBuffer)->upload(&frameData, sizeof(FrameData));

        cmd->bindDescriptorSet(frameResource.getDescriptorSet(FrameResourceId::MainDescriptorSet));
        planeRenderer.draw(cmd);
        sphereRenderer.draw(cmd);
        cmd->endRenderPass();

        cmd->present();

        device.endFrame(cmd);
    }

    device.waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
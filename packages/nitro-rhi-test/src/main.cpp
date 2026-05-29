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

    std::vector<RHIDescriptorBinding> binding = {{.type = RHIDescriptorBinding::Type::UniformBuffer,
                                                  .stage = RHIDescriptorBinding::ShaderStage::Vertex,
                                                  .binding = 2},
                                                 {.type = RHIDescriptorBinding::Type::UniformBuffer,
                                                  .stage = RHIDescriptorBinding::ShaderStage::Vertex,
                                                  .binding = 3},
                                                 {.type = RHIDescriptorBinding::Type::Sampler,
                                                  .stage = RHIDescriptorBinding::ShaderStage::Fragment,
                                                  .binding = 0}};

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
    Mesh plane = MeshGenerator::createPlane(50, 50);
    plane.calculateNormals();
    MeshRenderer planeRenderer(plane, &device);
    planeRenderer.transformation.translate(glm::vec3(0.0f, -10.0f, 0.0f));

    BufferDesc globalUBODesc;
    globalUBODesc.size = sizeof(GlobalTransformation);
    globalUBODesc.storage = BufferDesc::StorageMode::Shared;
    globalUBODesc.usage = BufferDesc::Usage::Uniform;

    RHIBuffer *uboBuffers[2];

    uboBuffers[0] = device.createBuffer(globalUBODesc);
    uboBuffers[1] = device.createBuffer(globalUBODesc);

    BufferDesc lightUboDesc;
    lightUboDesc.size = sizeof(LightView);
    lightUboDesc.storage = BufferDesc::StorageMode::Shared;
    lightUboDesc.usage = BufferDesc::Usage::Uniform;
    RHIBuffer *lightBuffers[2];
    lightBuffers[0] = device.createBuffer(lightUboDesc);
    lightBuffers[1] = device.createBuffer(lightUboDesc);

    OrbitalCamera camera;
    OrbitalCamera light;
    light.radius = 20.0f;
    light.theta = glm::radians(10.0f);
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
    shadowRenderPassDesc.depthAttachment = &shadowDepthAttachment;

    RHIRenderPass *shadowRenderPass = device.createRenderPass(shadowRenderPassDesc);
    RHIDescriptorSet *shadowDescriptorSets[2];
    shadowDescriptorSets[0] = device.createDescriptorSet(shadowDescriptorLayout);
    shadowDescriptorSets[0]->writeBuffer(lightBuffers[0], 2);
    shadowDescriptorSets[0]->commit();

    shadowDescriptorSets[1] = device.createDescriptorSet(shadowDescriptorLayout);
    shadowDescriptorSets[1]->writeBuffer(lightBuffers[1], 2);
    shadowDescriptorSets[1]->commit();

    RHIDescriptorSet *descriptorSets[2];
    descriptorSets[0] = device.createDescriptorSet(descriptorLayout);
    descriptorSets[0]->writeBuffer(uboBuffers[0], 2);
    descriptorSets[0]->writeBuffer(lightBuffers[0], 3);
    descriptorSets[0]->writeTexture(shadowDepthTexture, 0);
    descriptorSets[0]->commit();
    descriptorSets[1] = device.createDescriptorSet(descriptorLayout);
    descriptorSets[1]->writeBuffer(uboBuffers[1], 2);
    descriptorSets[1]->writeBuffer(lightBuffers[1], 3);
    descriptorSets[1]->writeTexture(shadowDepthTexture, 0);
    descriptorSets[1]->commit();
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        GlobalTransformation globalUbo{};
        int width, height;

        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;

        RHICommandBuffer *cmd = device.beginFrame();

        LightView lightView;
        lightView.lightSpaceMatrix = glm::orthoRH_ZO(-20.0f,
                                                     20.0f,
                                                     -20.0f,
                                                     20.0f,
                                                     0.1f,
                                                     50.0f) *
                                     light.getView();
        if (!isMetal)
        {
            lightView.lightSpaceMatrix[1][1] *= -1;
        }
        cmd->beginRenderPass(shadowRenderPass);
        cmd->bindPipeline(shadowPipeline);
        uint32_t frameIdx = device.getCurrentFrameIndex();
        RHIBuffer *lightBuffer = lightBuffers[frameIdx];

        lightBuffer->upload(&lightView, sizeof(LightView));
        cmd->bindDescriptorSet(shadowDescriptorSets[frameIdx]);
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
        cmd->beginRenderPass(rpDesc);
        cmd->bindPipeline(pipeline);
        RHIBuffer *uniformBuffer = uboBuffers[frameIdx];
        uniformBuffer->upload(&globalUbo, sizeof(GlobalTransformation));
        uniformBuffer->upload(&globalUbo, sizeof(GlobalTransformation));
        uniformBuffer->upload(&globalUbo, sizeof(GlobalTransformation));
        cmd->bindDescriptorSet(descriptorSets[frameIdx]);
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
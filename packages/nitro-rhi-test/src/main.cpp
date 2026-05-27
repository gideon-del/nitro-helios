#include <GLFW/glfw3.h>

#include <nitro-rhi/rhi-command-buffer.h>
#include <nitro-geometry/geometry.h>
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

constexpr float EPSILON = 1e-6f;

struct AppState
{
    OrbitalCamera *camera;
    bool mousePressed;
    double lastX, lastY;
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

    std::vector<RHIDescriptorBinding> binding = {{.type = RHIDescriptorBinding::Type::UniformBuffer,
                                                  .stage = RHIDescriptorBinding::ShaderStage::Vertex,
                                                  .binding = 2}};

    RHIDescriptorLayout *descriptorLayout = device.createDescriptorLayout(binding);
    PipelineDesc pipelineDesc{};
#ifdef USE_METAL
    pipelineDesc.vertexShader = {"vs", shaderPath + ".metallib"};
    pipelineDesc.fragmentShader = {"fs", shaderPath + ".metallib"};
#else
    pipelineDesc.vertexShader = {"main", shaderPath + ".vert.spv"};
    pipelineDesc.fragmentShader = {"main", shaderPath + ".frag.spv"};
#endif
    pipelineDesc.vertexLayout = Vertex::getVertexLayout();
    pipelineDesc.depthTest = true;
    pipelineDesc.layout = descriptorLayout;
    RHIPipeline *pipeline = device.createPipeline(pipelineDesc);
    pipelineDesc.topology = PipelineTopology::LineList;
    RHIPipeline *normalPipeline = device.createPipeline(pipelineDesc);
    Mesh ring = MeshGenerator::createUVSphere(5, 10, 100);
    ring.calculateNormals();
    Mesh ringNormals = MeshGenerator::createNormalVisualization(ring, 1);

    BufferDesc vertexDesc;
    vertexDesc.initialData = ring.vertices.data();
    vertexDesc.size = sizeof(Vertex) * ring.vertices.size();
    vertexDesc.storage = BufferDesc::StorageMode::GPU;
    vertexDesc.usage = BufferDesc::Usage::Vertex;

    RHIBuffer *vertexBuffer = device.createBuffer(vertexDesc);
    BufferDesc indexDesc;
    indexDesc.initialData = ring.indices.data();
    indexDesc.size = sizeof(uint32_t) * ring.indices.size();
    indexDesc.storage = BufferDesc::StorageMode::GPU;
    indexDesc.usage = BufferDesc::Usage::Index;

    RHIBuffer *indexBuffer = device.createBuffer(indexDesc);

    BufferDesc normalVertexDesc;
    normalVertexDesc.initialData = ringNormals.vertices.data();
    normalVertexDesc.size = sizeof(Vertex) * ringNormals.vertices.size();
    normalVertexDesc.storage = BufferDesc::StorageMode::GPU;
    normalVertexDesc.usage = BufferDesc::Usage::Vertex;

    RHIBuffer *normalVertexBuffer = device.createBuffer(normalVertexDesc);
    BufferDesc normalIndexDesc;
    normalIndexDesc.initialData = ringNormals.indices.data();
    normalIndexDesc.size = sizeof(uint32_t) * ringNormals.indices.size();
    normalIndexDesc.storage = BufferDesc::StorageMode::GPU;
    normalIndexDesc.usage = BufferDesc::Usage::Index;

    RHIBuffer *normalIndexBuffer = device.createBuffer(normalIndexDesc);

    BufferDesc globalUBODesc;
    globalUBODesc.size = sizeof(GlobalTransformation);
    globalUBODesc.storage = BufferDesc::StorageMode::Shared;
    globalUBODesc.usage = BufferDesc::Usage::Uniform;

    RHIBuffer *uboBuffers[2];

    uboBuffers[0] = device.createBuffer(globalUBODesc);
    uboBuffers[1] = device.createBuffer(globalUBODesc);

    RHIDescriptorSet *descriptorSets[2];
    descriptorSets[0] = device.createDescriptorSet(descriptorLayout);
    descriptorSets[0]->writeBuffer(uboBuffers[0], 2);
    descriptorSets[0]->commit();
    descriptorSets[1] = device.createDescriptorSet(descriptorLayout);
    descriptorSets[1]->writeBuffer(uboBuffers[1], 2);
    descriptorSets[1]->commit();

    OrbitalCamera camera;
    AppState appState{
        .camera = &camera};

    glfwSetWindowUserPointer(window, &appState);

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
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        nitro::rhi::PushConstant pushConstant;
        // pushConstant.model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.2f, 0.0f)), glm::radians(90.0f), glm::vec3{1.0f, 0.0f, 0.0f}), {3.0f, 3.0f, 1.0f});
        // glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3{1.0f, 0.0f, 0.0f});
        // glm::mat4 scale = glm::scale(glm::mat4(1.0f), {1.5f, 0.5f, 2.0f});
        // pushConstant.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f)) * scale;

        // pushConstant.model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        // pushConstant.model = glm::rotate(glm::mat4(1.0f),
        //                                  (float)glfwGetTime() * glm::radians(90.0f),
        //                                  glm::vec3(0.0f, 1.0f, 0.0f));
        pushConstant.model = glm::mat4(1.0f);
        pushConstant.applyNormalMatrix();
        // if (isMetal)
        // {
        //     pushConstant.model[1][1] *= -1;
        // }
        RHICommandBuffer *cmd = device.beginFrame();

        RHIRenderPassDesc rpDesc{};
        rpDesc.clearColor[0] = 0.2f;
        rpDesc.clearColor[1] = 0.2f;
        rpDesc.clearColor[2] = 0.2f;
        rpDesc.clearColor[3] = 1.0f;
        rpDesc.clearDepth = 1.0f;
        rpDesc.hasDepth = true;

        GlobalTransformation globalUbo{};
        int width, height;

        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (float)height;
        globalUbo.proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
        globalUbo.view = camera.getView();
        cmd->beginRenderPass(rpDesc);
        cmd->bindPipeline(pipeline);
        cmd->setPushConstant(&pushConstant, sizeof(nitro::rhi::PushConstant), 1);

        if (!isMetal)
        {
            globalUbo.proj[1][1] *= -1;
        }
        uint32_t frameIdx = device.getCurrentFrameIndex();
        RHIBuffer *uniformBuffer = uboBuffers[frameIdx];
        uniformBuffer->upload(&globalUbo, sizeof(GlobalTransformation));
        cmd->bindDescriptorSet(descriptorSets[frameIdx]);
        cmd->bindVertexBuffer(vertexBuffer);
        cmd->bindIndexBuffer(indexBuffer);
        cmd->drawIndexed(static_cast<uint32_t>(ring.indices.size()));

        cmd->bindPipeline(normalPipeline);
        cmd->bindVertexBuffer(normalVertexBuffer);
        cmd->bindIndexBuffer(normalIndexBuffer);
        cmd->drawIndexed(static_cast<uint32_t>(ringNormals.indices.size()));
        // pushConstant.model =
        //     glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // if (isMetal)
        // {
        //     pushConstant.model[1][1] *= -1;
        // }
        // cmd->setPushConstant(&pushConstant, sizeof(nitro::rhi::PushConstant), 1);
        // cmd->bindVertexBuffer(cubeVertexBuffer);
        // cmd->bindIndexBuffer(cubeIndexBuffer);
        // cmd->drawIndexed(36);

        cmd->endRenderPass();

        cmd->present();

        device.endFrame(cmd);
    }

    device.waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
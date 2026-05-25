#include <GLFW/glfw3.h>

#include <nitro-rhi/rhi-command-buffer.h>
#include <nitro-rhi-backends/common/push-constant.h>
#include <nitro-rhi-backends/common/vertex.h>
#include <nitro-rhi-backends/common/global-transformaton.h>
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

Vertex vertices[] = {
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}};

uint32_t indices[] = {0, 1, 2, 2, 3, 0};

Vertex cubeVertices[] = {
    // Front (Red)
    {{-0.5f, -0.5f, 0.5f}, {1, 0, 0}},
    {{0.5f, -0.5f, 0.5f}, {1, 0, 0}},
    {{0.5f, 0.5f, 0.5f}, {1, 0, 0}},
    {{-0.5f, 0.5f, 0.5f}, {1, 0, 0}},

    // Back (Green)
    {{-0.5f, -0.5f, -0.5f}, {0, 1, 0}},
    {{0.5f, -0.5f, -0.5f}, {0, 1, 0}},
    {{0.5f, 0.5f, -0.5f}, {0, 1, 0}},
    {{-0.5f, 0.5f, -0.5f}, {0, 1, 0}},

    // Left (Blue)
    {{-0.5f, -0.5f, -0.5f}, {0, 0, 1}},
    {{-0.5f, -0.5f, 0.5f}, {0, 0, 1}},
    {{-0.5f, 0.5f, 0.5f}, {0, 0, 1}},
    {{-0.5f, 0.5f, -0.5f}, {0, 0, 1}},

    // Right (Yellow)
    {{0.5f, -0.5f, -0.5f}, {1, 1, 0}},
    {{0.5f, -0.5f, 0.5f}, {1, 1, 0}},
    {{0.5f, 0.5f, 0.5f}, {1, 1, 0}},
    {{0.5f, 0.5f, -0.5f}, {1, 1, 0}},

    // Top (Magenta)
    {{-0.5f, 0.5f, -0.5f}, {1, 0, 1}},
    {{-0.5f, 0.5f, 0.5f}, {1, 0, 1}},
    {{0.5f, 0.5f, 0.5f}, {1, 0, 1}},
    {{0.5f, 0.5f, -0.5f}, {1, 0, 1}},

    // Bottom (Cyan)
    {{-0.5f, -0.5f, -0.5f}, {0, 1, 1}},
    {{-0.5f, -0.5f, 0.5f}, {0, 1, 1}},
    {{0.5f, -0.5f, 0.5f}, {0, 1, 1}},
    {{0.5f, -0.5f, -0.5f}, {0, 1, 1}},
};

uint32_t cubeIndices[] = {
    0,
    1,
    2,
    2,
    3,
    0, // Front
    4,
    5,
    6,
    6,
    7,
    4, // Back
    8,
    9,
    10,
    10,
    11,
    8, // Left
    12,
    13,
    14,
    14,
    15,
    12, // Right
    16,
    17,
    18,
    18,
    19,
    16, // Top
    20,
    21,
    22,
    22,
    23,
    20, // Bottom
};
;
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

    std::string shaderPath = std::string(SHADER_DIR) + "/cube";

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

    BufferDesc vertexDesc;
    vertexDesc.initialData = vertices;
    vertexDesc.size = sizeof(vertices);
    vertexDesc.storage = BufferDesc::StorageMode::GPU;
    vertexDesc.usage = BufferDesc::Usage::Vertex;

    RHIBuffer *vertexBuffer = device.createBuffer(vertexDesc);
    BufferDesc indexDesc;
    indexDesc.initialData = indices;
    indexDesc.size = sizeof(indices);
    indexDesc.storage = BufferDesc::StorageMode::GPU;
    indexDesc.usage = BufferDesc::Usage::Index;

    RHIBuffer *indexBuffer = device.createBuffer(indexDesc);

    BufferDesc cubeVertexDesc;
    cubeVertexDesc.initialData = cubeVertices;
    cubeVertexDesc.size = sizeof(cubeVertices);
    cubeVertexDesc.storage = BufferDesc::StorageMode::GPU;
    cubeVertexDesc.usage = BufferDesc::Usage::Vertex;

    RHIBuffer *cubeVertexBuffer = device.createBuffer(cubeVertexDesc);
    BufferDesc cubeIndexDesc;
    cubeIndexDesc.initialData = cubeIndices;
    cubeIndexDesc.size = sizeof(cubeIndices);
    cubeIndexDesc.storage = BufferDesc::StorageMode::GPU;
    cubeIndexDesc.usage = BufferDesc::Usage::Index;

    RHIBuffer *cubeIndexBuffer = device.createBuffer(cubeIndexDesc);

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

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        nitro::rhi::PushConstant pushConstant;
        pushConstant.model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.2f, 0.0f)), glm::radians(90.0f), glm::vec3{1.0f, 0.0f, 0.0f}), {3.0f, 3.0f, 1.0f});

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
        globalUbo.view = glm::lookAt(
            glm::vec3(0.0f, 3.0f, 5.0f),  // higher + further back
            glm::vec3(0.0f, 0.0f, -2.0f), // look at the cube position
            glm::vec3(0.0f, 1.0f, 0.0f));
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
        cmd->drawIndexed(6);

        pushConstant.model =
            glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // if (isMetal)
        // {
        //     pushConstant.model[1][1] *= -1;
        // }
        cmd->setPushConstant(&pushConstant, sizeof(nitro::rhi::PushConstant), 1);
        cmd->bindVertexBuffer(cubeVertexBuffer);
        cmd->bindIndexBuffer(cubeIndexBuffer);
        cmd->drawIndexed(36);

        cmd->endRenderPass();

        cmd->present();

        device.endFrame(cmd);
    }

    device.waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
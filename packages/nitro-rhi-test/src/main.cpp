#include <GLFW/glfw3.h>

#include <nitro-rhi/rhi-command-buffer.h>
#include <nitro-rhi-backends/common/push-constant.h>
#include <nitro-rhi-backends/common/vertex.h>
#include <nitro-rhi-backends/common/mesh.h>
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

Mesh createQuad(float width, float height)
{
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    float leftX = 0.0f - halfWidth;
    float rightX = 0.0f + halfWidth;

    float topY = 0.0f + halfHeight;
    float bottomY = 0.0f - halfHeight;

    Vertex v0{{leftX, topY, 0.0f}, {1.0f, 0.0f, 0.0f}};
    Vertex v1{{rightX, topY, 0.0f}, {0.0f, 1.0f, 0.0f}};
    Vertex v2{{rightX, bottomY, 0.0f}, {0.0f, 0.0f, 1.0f}};
    Vertex v3{{leftX, bottomY, 0.0f}, {1.0f, 0.0f, 0.0f}};

    return Mesh{
        .vertices = {v0, v1, v2, v3},
        .indices = {0, 1, 2, 2, 3, 0}};
}

Mesh createPlane(float width, float depth)
{

    float halfWidth = width * 0.5f;
    float halfDepth = depth * 0.5f;

    float leftX = 0.0f - halfWidth;
    float rightX = 0.0f + halfWidth;

    float farZ = 0.0f + halfDepth;
    float nearZ = 0.0f - halfDepth;

    Vertex v0{{leftX, 0.0f, farZ}, {1.0f, 0.0f, 0.0f}};
    Vertex v1{{rightX, 0.0f, farZ}, {0.0f, 1.0f, 0.0f}};
    Vertex v2{{rightX, 0.0f, nearZ}, {0.0f, 0.0f, 1.0f}};
    Vertex v3{{leftX, 0.0f, nearZ}, {1.0f, 0.0f, 0.0f}};

    return Mesh{
        .vertices = {v0, v1, v2, v3},
        .indices = {0, 1, 2, 2, 3, 0}};
}

Mesh createGrid(uint32_t rows, uint32_t cols, float width, float height)
{

    float widthPerCell = width / float(cols);
    float heightPerCell = height / float(rows);

    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    glm::vec3 leftTopPos{0.0f - halfWidth, 0.0f + halfHeight, 0.0f};

    Mesh mesh;

       for (int row = 0; row <= rows; row++)
    {
        float y = (float)row * heightPerCell;

        for (int col = 0; col <= cols; col++)
        {
            float x = (float)col * widthPerCell;
            uint32_t idx = col;
            float r = (idx + 0) % 3;
            float g = (idx + 1) % 3;
            float b = (idx + 2) % 3;
            mesh.vertices.push_back({leftTopPos + glm::vec3{x, -y, 0.0f},
                                     {r, g, b}});
        }
    }

    for (int col = 0; col < cols; col++)
    {
        for (int row = 0; row < rows; row++)
        {
            uint32_t topLeft = (row * (cols + 1)) + col;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = topLeft + (cols + 1);
            uint32_t bottomRight = bottomLeft + 1;
            mesh.indices.push_back(topLeft);
            mesh.indices.push_back(topRight);
            mesh.indices.push_back(bottomRight);

            mesh.indices.push_back(bottomRight);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topLeft);

            // Vertex topLeft{
            //     .pos = leftTopPos + glm::vec3(left * widthPerCell, -top * heightPerCell, 0.0f),
            //     .color = {1.0f, 0.0f, 0.0f}};
            // Vertex topRight{
            //     .pos = leftTopPos + glm::vec3(right * widthPerCell, -top * heightPerCell, 0.0f),
            //     .color = {0.0f, 1.0f, 0.0f}};
            // Vertex bottomRight{
            //     .pos = leftTopPos + glm::vec3(right * widthPerCell, -bottom * heightPerCell, 0.0f),
            //     .color = {0.0f, 0.0f, 1.0f}};
            // Vertex bottomLeft{
            //     .pos = leftTopPos + glm::vec3(left * widthPerCell, -bottom * heightPerCell, 0.0f),
            //     .color = {1.0f, 0.0f, 0.0f}};

            // uint32_t vertexSize = static_cast<uint32_t>(mesh.vertices.size());
            // mesh.vertices.push_back(topLeft);
            // mesh.vertices.push_back(topRight);
            // mesh.vertices.push_back(bottomRight);
            // mesh.vertices.push_back(bottomLeft);

            // uint32_t firstVertexIdx = vertexSize;
            // uint32_t secondVertexIdx = vertexSize + 1;
            // uint32_t thirdVertexIdx = vertexSize + 2;
            // uint32_t fourthVertexIdx = vertexSize + 3;
            // mesh.indices.push_back(firstVertexIdx);
            // mesh.indices.push_back(secondVertexIdx);
            // mesh.indices.push_back(thirdVertexIdx);
            // mesh.indices.push_back(thirdVertexIdx);
            // mesh.indices.push_back(fourthVertexIdx);
            // mesh.indices.push_back(firstVertexIdx);
        }
    }

    return mesh;
}
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

    Mesh quad = createQuad(1.0f, 1.0f);
    Mesh plane = createPlane(1.0f, 1.0f);
    Mesh gridCell = createGrid(8, 8, 1.0f, 1.0f);
    BufferDesc vertexDesc;
    vertexDesc.initialData = gridCell.vertices.data();
    vertexDesc.size = sizeof(Vertex) * gridCell.vertices.size();
    vertexDesc.storage = BufferDesc::StorageMode::GPU;
    vertexDesc.usage = BufferDesc::Usage::Vertex;

    RHIBuffer *vertexBuffer = device.createBuffer(vertexDesc);
    BufferDesc indexDesc;
    indexDesc.initialData = gridCell.indices.data();
    indexDesc.size = sizeof(uint32_t) * gridCell.indices.size();
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
        // pushConstant.model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.2f, 0.0f)), glm::radians(90.0f), glm::vec3{1.0f, 0.0f, 0.0f}), {3.0f, 3.0f, 1.0f});
        // glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3{1.0f, 0.0f, 0.0f});
        // glm::mat4 scale = glm::scale(glm::mat4(1.0f), {1.5f, 0.5f, 2.0f});
        // pushConstant.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f)) * scale;

        pushConstant.model = glm::mat4(1.0f);
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
            glm::vec3(0.0f, 1.0f, -2.0f), // higher + further back
            glm::vec3(0.0f, 0.0f, 0.0f),  // look at the cube position
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
        cmd->drawIndexed(static_cast<uint32_t>(gridCell.indices.size()));

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
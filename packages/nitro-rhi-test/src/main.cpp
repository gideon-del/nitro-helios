#include <GLFW/glfw3.h>
#include <nitro-rhi-backends/vulkan/vulkan-device.h>
#include <nitro-rhi-backends/vulkan/vulkan-pipeline.h>
#include <nitro-rhi/rhi-command-buffer.h>
#include <iostream>
#include <string>

using namespace nitro::rhi;
using namespace nitro::rhi::vulkan;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(800, 600, "RHI Triangle", nullptr, nullptr);

    VulkanDevice device(window);
    RHISwapchain *swapchain = device.createSwapchain(nullptr);

    PipelineDesc pipelineDesc;
    auto vertexPath = std::string(SHADER_DIR) + "/triangle.vert.spv";
    auto fragmentPath = std::string(SHADER_DIR) + "/triangle.frag.spv";
    pipelineDesc.vertexShader = {"main", vertexPath.c_str()};
    pipelineDesc.fragmentShader = {"main", fragmentPath.c_str()};
    pipelineDesc.vertexLayout.attributes = {};
    pipelineDesc.vertexLayout.stride = 0;
    pipelineDesc.depthTest = true;

    RHIPipeline *pipeline = device.createPipeline(pipelineDesc);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        RHICommandBuffer *cmd = device.beginFrame();

        RHIRenderPassDesc rpDesc{};
        rpDesc.clearColor[0] = 0.2f;
        rpDesc.clearColor[1] = 0.2f;
        rpDesc.clearColor[2] = 0.2f;
        rpDesc.clearColor[3] = 1.0f;
        rpDesc.clearDepth = 1.0f;
        rpDesc.hasDepth = true;

        cmd->beginRenderPass(rpDesc);
        cmd->bindPipeline(pipeline);

        cmd->drawIndexed(3);

        cmd->endRenderPass();

        cmd->present();

        device.endFrame(cmd);
    }

    device.waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
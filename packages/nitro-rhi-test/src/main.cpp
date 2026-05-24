#include <GLFW/glfw3.h>

#include <nitro-rhi/rhi-command-buffer.h>
#include <nitro-rhi-backends/common/push-constant.h>
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

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(800, 600, "RHI Triangle", nullptr, nullptr);

    DeviceType device(window);
    RHISwapchain *swapchain = device.createSwapchain(nullptr);

    std::string shaderPath = std::string(SHADER_DIR) + "/triangle";

    PipelineDesc pipelineDesc{};
#ifdef USE_METAL
    pipelineDesc.vertexShader = {"vs", shaderPath + ".metallib"};
    pipelineDesc.fragmentShader = {"fs", shaderPath + ".metallib"};
#else
    pipelineDesc.vertexShader = {"main", shaderPath + ".vert.spv"};
    pipelineDesc.fragmentShader = {"main", shaderPath + ".frag.spv"};
#endif
    pipelineDesc.vertexLayout.attributes = {};
    pipelineDesc.vertexLayout.stride = 0;
    pipelineDesc.depthTest = true;

    RHIPipeline *pipeline = device.createPipeline(pipelineDesc);

    nitro::rhi::PushConstant pushConstant;
    pushConstant.model = glm::translate(glm::mat4(1.0f), glm::vec3{0.4f, 0.0f, 0.0f});
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
        cmd->setPushConstant(&pushConstant, sizeof(nitro::rhi::PushConstant), 1);
        cmd->draw(3);

        cmd->endRenderPass();

        cmd->present();

        device.endFrame(cmd);
    }

    device.waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
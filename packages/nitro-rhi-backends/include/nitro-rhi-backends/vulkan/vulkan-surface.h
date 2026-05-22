#pragma once
#include <nitro-rhi/rhi-surface.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
namespace nitro::rhi::vulkan
{
    class VulkanSurface : public RHISurface
    {
    public:
        VulkanSurface(VkInstance instance, void *window);
        ~VulkanSurface() override;
        uint32_t getWidth() const override;
        uint32_t getHeight() const override;

        VkInstance instance;
        VkSurfaceKHR surface;
        GLFWwindow *window;
    };
}
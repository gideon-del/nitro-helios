#include <nitro-rhi-backends/vulkan/vulkan-surface.h>
#include <nitro-rhi-backends/vulkan/vulkan-utils.h>
namespace nitro::rhi::vulkan
{
    VulkanSurface::VulkanSurface(VkInstance instance, void *windowHandle) : instance(instance)
    {
        window = reinterpret_cast<GLFWwindow *>(windowHandle);

        checkVkResult(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Surface not Created");
    }

    uint32_t VulkanSurface::getWidth() const
    {
        int width;
        glfwGetFramebufferSize(window, &width, nullptr);

        return static_cast<uint32_t>(width);
    }
    uint32_t VulkanSurface::getHeight() const
    {
        int height;
        glfwGetFramebufferSize(window, nullptr, &height);

        return static_cast<uint32_t>(height);
    }

    VulkanSurface::~VulkanSurface()
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }

}
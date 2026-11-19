#include "nitro-studio/device-factory.h"
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include "nitro-rhi-backends/metal/metal-device.h"
#endif
#include "nitro-rhi-backends/vulkan/vulkan-device.h"
#include <stdexcept>
namespace nitro::studio
{
    Backend defaultBackend()
    {
#ifdef __APPLE__
        // Metal Backend isn't stable but would be worked on after the Slang Shader transition
        return Backend::Vulkan;
#else
        return Backend::Vulkan;
#endif
    }

    std::shared_ptr<rhi::RHIDevice> createDevice(Backend backend, GLFWwindow *window)
    {
        switch (backend)
        {
        case Backend::Metal:
#ifdef __APPLE__
            return std::make_shared<rhi::metal::MetalDevice>(window);
#else
            throw std::runtime_error("Metal can only run on apple systems");
#endif
        case Backend::Vulkan:
            return std::make_shared<rhi::vulkan::VulkanDevice>(window);
        }

        throw std::runtime_error("Unknown backend");
    }
} // namespace nitro::studio

#pragma once
#include <GLFW/glfw3.h>
#include "nitro-rhi/rhi-device.h"

namespace nitro::studio
{
    enum class Backend
    {
        Vulkan,
        Metal
    };
    Backend defaultBackend();
    Backend backendFromArgs(int argc, char **argv);
    std::shared_ptr<rhi::RHIDevice> createDevice(Backend backend, GLFWwindow *window);
}
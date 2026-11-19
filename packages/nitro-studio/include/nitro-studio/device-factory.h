#pragma once
#include "nitro-rhi/rhi-device.h"

struct GLFWwindow;
namespace nitro::studio
{
    enum class Backend
    {
        Vulkan,
        Metal
    };
    Backend defaultBackend();
    // Backend backendFromArgs(int argc, char **argv);
    std::shared_ptr<rhi::RHIDevice> createDevice(Backend backend, GLFWwindow *window);
}
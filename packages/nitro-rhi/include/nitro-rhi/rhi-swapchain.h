#pragma once
#include "rhi-texture.h"
#include <cstdint>
namespace nitro::rhi
{
    class RHISwapchain
    {
    public:
        virtual ~RHISwapchain() = default;
        virtual void resize(uint32_t width, uint32_t height) = 0;
        virtual RHITexture *getCurrentBackbuffer() = 0;
    };
}